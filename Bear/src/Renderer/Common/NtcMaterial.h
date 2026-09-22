#pragma once
#include "Material.h"
#include "NtcChannelLayout.h"
#include "libntc/ntc.h"

#include <array>
#include <memory>
#include <vector>

namespace Bear
{
	// Background compression job, defined in NtcMaterial.cpp.
	struct NtcCompressionJob;

	class NtcMaterial : public Material
	{
	public:
		NtcMaterial(RHIDevice& device, const MaterialDescription& desc, const std::unordered_map<int, std::shared_ptr<Texture>>& textures);
		~NtcMaterial() override;
		RHIDescriptorSet* GetDescriptorSet() override { return m_DescriptorSets.get(); }
		RHIDescriptorSetLayout* GetDescriptorSetLayout() override { return m_DescriptorSetLayout.get(); }
		void SetBuffer(NtcMaterialSlot slot, size_t dataSize, const void* data);
		static std::vector<RHIDescriptorSetLayoutBinding> GetDescriptorSetLayoutBinding();
		// Compression runs on a background thread; the first call after it finishes performs the
		// GPU upload on the calling (main) thread and then reports ready.
		bool IsReady() override;
		// Bump when the channel layout changes: it is part of the compressed file name, so old
		// results are never reused.
		static constexpr int kLayoutVersion = 2;

		// Debug helper: semantic component -> texture set channel (-1 means a constant).
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
		std::shared_ptr<NtcCompressionJob> m_CompressionJob;
		bool m_Uploaded = false;
	private:
		void BuildChannelPlan(const MaterialDescription& desc, const std::unordered_map<int, std::shared_ptr<Texture>>& textures);
		void UploadTextures();
		std::array<ntc::ShuffleSource, NTC_MAX_CHANNELS> BuildShuffleMap() const;
		void EnsureContext();
		bool LoadCompressedFile(const std::string& filePath);
		void StartCompression(const MaterialDescription& desc, const std::unordered_map<int, std::shared_ptr<Texture>>& textures,
			const std::string& filePath, int width, int height, int mips);
		std::string GetCompressedFilePath(const MaterialDescription& desc, const std::unordered_map<int, std::shared_ptr<Texture>>& textures, int mipCount) const;
		void LogTextureSet(const char* stage) const;
	};
}
