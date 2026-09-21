#pragma once
#include "Material.h"
#include "NtcChannelLayout.h"
#include "libntc/ntc.h"

#include <array>
#include <vector>

namespace Bear
{
	class NtcMaterial : public Material
	{
	public:
		NtcMaterial(RHIDevice& device, const MaterialDescription& desc, const std::unordered_map<int, std::shared_ptr<Texture>>& textures);
		~NtcMaterial() override = default;
		RHIDescriptorSet* GetDescriptorSet() override { return m_DescriptorSets.get(); }
		RHIDescriptorSetLayout* GetDescriptorSetLayout() override { return m_DescriptorSetLayout.get(); }
		void SetBuffer(NtcMaterialSlot slot, size_t dataSize, const void* data);
		static std::vector<RHIDescriptorSetLayoutBinding> GetDescriptorSetLayoutBinding();
		// 写入通道的排布方式发生变化时必须 +1：它参与压缩结果的文件名，旧的压缩结果不会被复用。
		static constexpr int kLayoutVersion = 2;

		// 调试用：语义分量 -> 纹理集通道（-1 表示该语义走常量）。
		const NtcChannelPlan& GetChannelPlan() const { return m_ChannelPlan; }
	public:
		std::shared_ptr<RHIBuffer> m_ConstantBuffer;
		std::shared_ptr<RHIBuffer> m_LatentBuffer;
		std::shared_ptr<RHIBuffer> m_WeightBuffer;

		int networkVersion = 0;
		int weightType = 0;
		size_t memorySize = 0;
		
	private:
		RHIDevice& m_Device;
		std::shared_ptr<RHIDescriptorSetLayout> m_DescriptorSetLayout;
		static std::vector<RHIDescriptorSetLayoutBinding> s_DescriptorSetLayoutBinding;
		std::unique_ptr<RHIDescriptorSet> m_DescriptorSets;
		std::vector<uint8_t> m_CompressedData;
		ntc::IContext* m_NtcContext = nullptr;
		ntc::TextureSetWrapper m_TextureSet;
		NtcChannelPlan m_ChannelPlan;
		NtcSlotConstants m_SlotConstants;
	private:
		void BuildChannelPlan(const MaterialDescription& desc, const std::unordered_map<int, std::shared_ptr<Texture>>& textures);
		void CompressTextures(const MaterialDescription& desc, const std::unordered_map<int, std::shared_ptr<Texture>>& textures);
		void UploadTextures();
		std::array<ntc::ShuffleSource, NTC_MAX_CHANNELS> BuildShuffleMap() const;
		std::string GetCompressedFilePath(const MaterialDescription& desc, const std::unordered_map<int, std::shared_ptr<Texture>>& textures) const;
		void LogTextureSet(const char* stage) const;
	};
}
