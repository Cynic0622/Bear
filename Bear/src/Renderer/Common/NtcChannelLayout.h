#pragma once
// ============================================================================
// NTC 通道布局契约
//
// LibNTC 里的纹理集只是一段按顺序排列的通道：网络输出的第 i 个通道，就对应
// WriteChannels 写进去的第 i 个通道。而渲染端(ntcPS.hlsl)读取的是一组固定的
// 语义槽位（见 NtcChannelMapping.h），两者的对应关系由 ShuffleInferenceOutputs
// 在装载时重排产生。
//
// 于是"写入的通道顺序"和"读取的语义槽位"必须来自同一处定义。以前写入顺序是
// 由源贴图的文件通道数累加得到的（RGBA 占 4 格、灰度占 1 格），而槽位是硬编码
// 常量，一旦两者不一致就会静默错位：材质照常压缩、照常渲染，只是各个通道的
// 语义全错了。
//
// 这个文件就是那个唯一定义：
//   BuildNtcChannelPlan()  ->  写入计划（每次 WriteChannels 的源/目标通道段）
//   同一份计划             ->  shuffle 映射（语义槽位 -> 纹理集通道）
// ============================================================================

#include "NtcChannelMapping.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace Bear
{
	// 材质的语义通道。槽位号定义在 NtcChannelMapping.h。
	enum class NtcChannelRole : uint8_t
	{
		BaseColor = 0,
		Opacity,
		Metalness,
		Roughness,
		Occlusion,
		Normal,
		Emissive,
		Count
	};

	inline constexpr int kNtcChannelRoleCount = static_cast<int>(NtcChannelRole::Count);
	inline constexpr int kNtcRoleMaxComponents = 3; // 基色/法线/自发光是 3 分量，其余是 1 分量

	// 材质里实际存在的一张源贴图。
	struct NtcSourceTexture
	{
		NtcChannelRole role = NtcChannelRole::BaseColor;
		int textureIndex = -1;
		// 图片文件本身的通道数（1..4）。注意它描述的是源数据里"有意义的分量数"，
		// 而 Texture::GetImageData() 永远按 RGBA 补齐成 4 字节/像素。
		uint8_t channels = 0;
	};

	// 一次 ITextureSet::WriteChannels 调用。
	struct NtcChannelWrite
	{
		int textureIndex = -1;   // -1 表示不来自任何贴图（占位用的哑通道）
		int localChannel = 0;    // 从源图（RGBA 补齐后）的第几个分量开始取
		int numChannels = 0;
		int firstChannel = 0;    // 落在 NTC 纹理集里的起始通道
	};

	inline std::array<std::array<int, kNtcRoleMaxComponents>, kNtcChannelRoleCount> MakeEmptySourceChannelTable()
	{
		std::array<std::array<int, kNtcRoleMaxComponents>, kNtcChannelRoleCount> table{};
		for (auto& row : table)
			row.fill(-1);
		return table;
	}

	// 通道布局计划。
	struct NtcChannelPlan
	{
		std::vector<NtcChannelWrite> writes;
		// sourceChannel[role][component] = 纹理集里的通道号；-1 表示该语义没有源数据，
		// 渲染时用常量代替。
		std::array<std::array<int, kNtcRoleMaxComponents>, kNtcChannelRoleCount> sourceChannel = MakeEmptySourceChannelTable();
		int totalChannels = 0;
	};

	// 语义槽位缺失时写入的常量。注意这些值处在"纹理集存储空间"里：着色器拿到后
	// 直接使用（法线是 *2-1 之前的值，所以中性法线是 (0.5, 0.5, 1)）。
	struct NtcSlotConstants
	{
		float baseColor[3] = { 1.f, 1.f, 1.f };
		float opacity = 1.f;
		float metalness = 1.f;
		float roughness = 1.f;
		float occlusion = 1.f;
		float normal[3] = { 0.5f, 0.5f, 1.f };
		float emissive[3] = { 0.f, 0.f, 0.f };
		float transmission = 0.f;
	};

	// 线性 -> sRGB 编码。着色器读基色/自发光时不做解码（沿用纹理路径的存储值），
	// 所以只有因子、没有贴图时要把线性因子编码成同样的存储值。
	inline float NtcEncodeSrgb(float linear)
	{
		if (linear <= 0.f)
			return 0.f;
		if (linear >= 1.f)
			return 1.f;
		return linear <= 0.0031308f ? linear * 12.92f
		                            : 1.055f * std::pow(linear, 1.f / 2.4f) - 0.055f;
	}

	// 语义分量对应的 NTC 推理输出槽位；-1 表示该分量没有独立槽位。
	inline int NtcCanonicalSlot(NtcChannelRole role, int component)
	{
		switch (role)
		{
		case NtcChannelRole::BaseColor: return CHANNEL_BASE_COLOR + component;
		case NtcChannelRole::Opacity:   return component == 0 ? CHANNEL_OPACITY : -1;
		case NtcChannelRole::Metalness: return component == 0 ? CHANNEL_METALNESS : -1;
		case NtcChannelRole::Roughness: return component == 0 ? CHANNEL_ROUGHNESS : -1;
		case NtcChannelRole::Occlusion: return component == 0 ? CHANNEL_OCCLUSION : -1;
		case NtcChannelRole::Normal:    return CHANNEL_NORMAL + component;
		case NtcChannelRole::Emissive:  return CHANNEL_EMISSIVE + component;
		default:                        return -1;
		}
	}

	inline float NtcDefaultChannelValue(NtcChannelRole role, int component, const NtcSlotConstants& constants)
	{
		switch (role)
		{
		case NtcChannelRole::BaseColor: return constants.baseColor[component];
		case NtcChannelRole::Opacity:   return constants.opacity;
		case NtcChannelRole::Metalness: return constants.metalness;
		case NtcChannelRole::Roughness: return constants.roughness;
		case NtcChannelRole::Occlusion: return constants.occlusion;
		case NtcChannelRole::Normal:    return constants.normal[component];
		case NtcChannelRole::Emissive:  return constants.emissive[component];
		default:                        return 0.f;
		}
	}

	// ------------------------------------------------------------------------
	// 生成写入计划。
	//
	// sources 需要按语义给出材质里实际存在的源贴图（不存在的语义不要放进来）。
	// sources 的顺序不影响结果，同一个 textureIndex 出现多次是允许的（ORM 打包的
	// 材质里 occlusion 与 metallicRoughness 常常指向同一张图）。
	//
	// useOpacity：材质是否会用到基色的 alpha（glTF alphaMode == BLEND）。为 false 时
	// 不会为 alpha 单独占用通道。
	// ------------------------------------------------------------------------
	inline NtcChannelPlan BuildNtcChannelPlan(const std::vector<NtcSourceTexture>& sources, bool useOpacity)
	{
		NtcChannelPlan plan;

		auto find = [&sources](NtcChannelRole role) -> const NtcSourceTexture*
		{
			for (const auto& source : sources)
			{
				if (source.role == role && source.textureIndex >= 0 && source.channels > 0)
					return &source;
			}
			return nullptr;
		};

		auto addWrite = [&plan](int textureIndex, int localChannel, int numChannels) -> int
		{
			NtcChannelWrite write;
			write.textureIndex = textureIndex;
			write.localChannel = localChannel;
			write.numChannels = numChannels;
			write.firstChannel = plan.totalChannels;
			plan.writes.push_back(write);
			plan.totalChannels += numChannels;
			return write.firstChannel;
		};

		auto setSource = [&plan](NtcChannelRole role, int component, int channel)
		{
			plan.sourceChannel[static_cast<int>(role)][component] = channel;
		};

		// --- 基色（+ 可选的不透明） ---
		if (const NtcSourceTexture* base = find(NtcChannelRole::BaseColor))
		{
			const bool hasAlpha = (base->channels == 4 || base->channels == 2);
			if (base->channels >= 3)
			{
				// RGB 与 alpha 在 RGBA 数据里是连续的，可以合并成一次写入。
				const int count = (useOpacity && base->channels == 4) ? 4 : 3;
				const int first = addWrite(base->textureIndex, 0, count);
				for (int c = 0; c < 3; ++c)
					setSource(NtcChannelRole::BaseColor, c, first + c);
				if (count == 4)
					setSource(NtcChannelRole::Opacity, 0, first + 3);
			}
			else
			{
				// 灰度（1 通道）或灰度+alpha（2 通道）：RGB 由同一个通道广播得到。
				const int firstGray = addWrite(base->textureIndex, 0, 1);
				for (int c = 0; c < 3; ++c)
					setSource(NtcChannelRole::BaseColor, c, firstGray);
				if (useOpacity && hasAlpha)
					setSource(NtcChannelRole::Opacity, 0, addWrite(base->textureIndex, 1, 1));
			}
		}

		// --- 金属度 / 粗糙度 / 遮蔽 ---
		const NtcSourceTexture* metalRough = find(NtcChannelRole::Metalness);
		const NtcSourceTexture* occlusion = find(NtcChannelRole::Occlusion);
		// glTF 里 metallicRoughness 贴图是 G=粗糙度、B=金属度，R 通道是**未使用的**；遮蔽只可能
		// 来自 occlusionTexture。ORM 打包的资产会把 occlusionTexture 指向同一张图，此时它的 R
		// 通道才是遮蔽。没有 occlusionTexture 时不写遮蔽通道，着色器取常量 1.0 —— 与 pbr.frag
		// 在没有遮蔽贴图时绑定 1x1 白色默认贴图的行为一致。
		const bool occlusionFromMetalRough = (occlusion != nullptr) && (metalRough != nullptr) &&
			(occlusion->textureIndex == metalRough->textureIndex);
		if (metalRough)
		{
			if (metalRough->channels >= 3)
			{
				const int localChannel = occlusionFromMetalRough ? 0 : 1;
				const int count = occlusionFromMetalRough ? 3 : 2;
				const int first = addWrite(metalRough->textureIndex, localChannel, count);
				if (occlusionFromMetalRough)
					setSource(NtcChannelRole::Occlusion, 0, first + 0);
				setSource(NtcChannelRole::Roughness, 0, first + (occlusionFromMetalRough ? 1 : 0));
				setSource(NtcChannelRole::Metalness, 0, first + (occlusionFromMetalRough ? 2 : 1));
			}
			else
			{
				// 单通道图按粗糙度处理（金属度用常量）。
				setSource(NtcChannelRole::Roughness, 0, addWrite(metalRough->textureIndex, 0, 1));
			}
		}
		if (occlusion && !occlusionFromMetalRough)
		{
			// 独立的遮蔽贴图：只取它的 R 通道。
			setSource(NtcChannelRole::Occlusion, 0, addWrite(occlusion->textureIndex, 0, 1));
		}

		// --- 法线 ---
		if (const NtcSourceTexture* normal = find(NtcChannelRole::Normal))
		{
			if (normal->channels >= 3)
			{
				const int first = addWrite(normal->textureIndex, 0, 3);
				for (int c = 0; c < 3; ++c)
					setSource(NtcChannelRole::Normal, c, first + c);
			}
			else if (normal->channels == 2)
			{
				// 双通道法线（X、Y），Z 用中性常量。
				const int first = addWrite(normal->textureIndex, 0, 2);
				setSource(NtcChannelRole::Normal, 0, first + 0);
				setSource(NtcChannelRole::Normal, 1, first + 1);
			}
			// 单通道数据没有明确语义，按缺失处理、走中性常量。
		}

		// --- 自发光 ---
		if (const NtcSourceTexture* emissive = find(NtcChannelRole::Emissive))
		{
			if (emissive->channels >= 3)
			{
				const int first = addWrite(emissive->textureIndex, 0, 3);
				for (int c = 0; c < 3; ++c)
					setSource(NtcChannelRole::Emissive, c, first + c);
			}
			else
			{
				const int first = addWrite(emissive->textureIndex, 0, 1);
				for (int c = 0; c < 3; ++c)
					setSource(NtcChannelRole::Emissive, c, first);
			}
		}

		return plan;
	}
}
