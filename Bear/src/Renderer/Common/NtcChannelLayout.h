#pragma once
// ============================================================================
// NTC channel layout contract
//
// A LibNTC texture set is just an ordered run of channels: inference output i
// corresponds to the i-th channel written with WriteChannels. The shader (ntcPS)
// reads a fixed set of semantic slots (see NtcChannelMapping.h) and
// ShuffleInferenceOutputs rewires one into the other at load time.
//
// The write order and the slot mapping therefore have to come from one definition.
// Deriving the write order from each image's channel count (4 for RGBA, 1 for
// grayscale) while hardcoding the slots silently shifted every channel after the
// first non-RGB map: materials still compressed and rendered, just with the wrong
// meaning per channel.
//
// This file is that single definition:
//   BuildNtcChannelPlan()  ->  write plan (source/target ranges per WriteChannels)
//   the same plan          ->  shuffle map (semantic slot -> texture set channel)
// ============================================================================

#include "NtcChannelMapping.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace Bear
{
	// Semantic channels of a material. Slot numbers live in NtcChannelMapping.h.
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
	inline constexpr int kNtcRoleMaxComponents = 3; // base color/normal/emissive are 3, the rest 1

	// One source texture that actually exists in the material.
	struct NtcSourceTexture
	{
		NtcChannelRole role = NtcChannelRole::BaseColor;
		int textureIndex = -1;
		// Meaningful components in the source data (1..4). Note that
		// Texture::GetImageData() is always padded to 4 bytes per pixel.
		uint8_t channels = 0;
	};

	// One ITextureSet::WriteChannels call.
	struct NtcChannelWrite
	{
		int textureIndex = -1;   // -1: not from a texture (dummy channel used for placeholder sets)
		int localChannel = 0;    // first component to take from the padded source image
		int numChannels = 0;
		int firstChannel = 0;    // first channel in the NTC texture set
	};

	inline std::array<std::array<int, kNtcRoleMaxComponents>, kNtcChannelRoleCount> MakeEmptySourceChannelTable()
	{
		std::array<std::array<int, kNtcRoleMaxComponents>, kNtcChannelRoleCount> table{};
		for (auto& row : table)
			row.fill(-1);
		return table;
	}

	// Channel layout plan.
	struct NtcChannelPlan
	{
		std::vector<NtcChannelWrite> writes;
		// sourceChannel[role][component] = channel in the texture set, or -1 when the semantic has
		// no source data (the shader then uses a constant).
		std::array<std::array<int, kNtcRoleMaxComponents>, kNtcChannelRoleCount> sourceChannel = MakeEmptySourceChannelTable();
		int totalChannels = 0;
	};

	// Constants used when a semantic has no source data. These are values in texture set storage
	// space: the shader uses them as-is (normals are pre *2-1, so neutral is (0.5, 0.5, 1)).
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

	// Linear -> sRGB. The shader consumes base color/emissive as stored values without decoding, so
	// factors used in place of a texture have to be encoded the same way.
	inline float NtcEncodeSrgb(float linear)
	{
		if (linear <= 0.f)
			return 0.f;
		if (linear >= 1.f)
			return 1.f;
		return linear <= 0.0031308f ? linear * 12.92f
		                            : 1.055f * std::pow(linear, 1.f / 2.4f) - 0.055f;
	}

	// NTC inference slot for a semantic component, or -1 when it has no dedicated slot.
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
	// Builds the write plan.
	//
	// 'sources' lists the source textures the material actually has (skip missing semantics).
	// Order does not matter; the same textureIndex may appear twice (ORM-packed materials point
	// occlusion and metallicRoughness at the same image).
	//
	// 'useOpacity': whether base color alpha is used (glTF alphaMode == BLEND). When false no
	// channel is reserved for alpha.
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

		// --- base color (+ optional opacity) ---
		if (const NtcSourceTexture* base = find(NtcChannelRole::BaseColor))
		{
			const bool hasAlpha = (base->channels == 4 || base->channels == 2);
			if (base->channels >= 3)
			{
				// RGB and alpha are contiguous in RGBA data: one write covers both.
				const int count = (useOpacity && base->channels == 4) ? 4 : 3;
				const int first = addWrite(base->textureIndex, 0, count);
				for (int c = 0; c < 3; ++c)
					setSource(NtcChannelRole::BaseColor, c, first + c);
				if (count == 4)
					setSource(NtcChannelRole::Opacity, 0, first + 3);
			}
			else
			{
				// Grayscale (1 channel) or grayscale+alpha (2): RGB is broadcast from one channel.
				const int firstGray = addWrite(base->textureIndex, 0, 1);
				for (int c = 0; c < 3; ++c)
					setSource(NtcChannelRole::BaseColor, c, firstGray);
				if (useOpacity && hasAlpha)
					setSource(NtcChannelRole::Opacity, 0, addWrite(base->textureIndex, 1, 1));
			}
		}

		// --- metalness / roughness / occlusion ---
		const NtcSourceTexture* metalRough = find(NtcChannelRole::Metalness);
		const NtcSourceTexture* occlusion = find(NtcChannelRole::Occlusion);
		// glTF metallicRoughness packs G=roughness, B=metalness and leaves R unused; occlusion only
		// ever comes from occlusionTexture. ORM-packed assets point that texture at the same image,
		// and then its R channel is the AO. Without an occlusion texture there is no AO channel and
		// the shader uses the constant 1.0, matching pbr.frag's white default texture.
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
				// Single channel image: treat it as roughness (metalness stays a constant).
				setSource(NtcChannelRole::Roughness, 0, addWrite(metalRough->textureIndex, 0, 1));
			}
		}
		if (occlusion && !occlusionFromMetalRough)
		{
			// Separate occlusion texture: take its R channel only.
			setSource(NtcChannelRole::Occlusion, 0, addWrite(occlusion->textureIndex, 0, 1));
		}

		// --- normal ---
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
				// Two-channel normal (X, Y): Z falls back to the neutral constant.
				const int first = addWrite(normal->textureIndex, 0, 2);
				setSource(NtcChannelRole::Normal, 0, first + 0);
				setSource(NtcChannelRole::Normal, 1, first + 1);
			}
			// One channel has no defined meaning here: treat the normal as missing.
		}

		// --- emissive ---
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
