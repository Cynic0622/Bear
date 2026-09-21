#pragma once
#include <memory>
#include <vector>

#include "Mesh.h"
#include "Texture.h"
#include "BaseData.h"
#include "RenderStats.h"
#include "RenderGraph.h"

namespace Bear
{
	class OitPass;
	class PbrPass;
	class SkyboxPass;
	class CullingPass;
	class HiZPass;
	class ShadowPass;
}

struct GLFWwindow;
namespace Bear {
	class UIPass;

	enum class GraphicsAPI;
	class RHIDevice;
	class RHISwapchain;
	class RHIRenderPass;
	struct RenderObject;
	class ResourceManager;

	class BEAR_API Renderer {
	public:
		Renderer(GLFWwindow* window, GraphicsAPI api);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer&& operator=(Renderer&&) = delete;

		bool OnWindowResize() const;
		bool OnKeyPress();
		void OnEvent(Event& event);

		// getter
		RHIDevice* GetDevice() const { return m_Device.get(); }
		RHICommandList* GetCurrentCommandList() const { return m_CurrentCommandBuffer; }
		uint32_t GetSwapchainImageCount() const;
		ResourceManager& GetResourceManager() { return *m_Resource; }
		const RenderStats& GetFrameStats() const { return m_FrameStats; }
		bool IsOitEnabled() const { return OitEnabled; }
		bool IsGpuCullingEnabled() const;
		bool IsOcclusionCullingEnabled() const;
		bool IsTextureCompressionEnabled() const { return m_TextureCompressionEnabled; }

		// Toggles neural texture compression. Rebuilds the PBR pipeline and the set 2 layout;
		// the caller still has to recreate the materials (SceneLayer::ReloadModel) because NTC
		// materials are compressed when they are created.
		void SetTextureCompressionEnabled(bool enabled);

		void BeginFrame();
		void Submit(const std::vector<RenderObject>& renderObjects, const SceneData& sceneData, const BaseData& baseData);
		void EndFrame();


	private:
		void Init(GraphicsAPI api);

	private:
		GLFWwindow* m_Window;
		const uint8_t MAX_FRAMES_IN_FLIGHT = 2;
		std::unique_ptr<RHIDevice> m_Device;
		std::unique_ptr<RHISwapchain> m_Swapchain;

		RHICommandList* m_CurrentCommandBuffer;
		uint32_t m_CurrentImageIndex;
		std::unique_ptr<ResourceManager> m_Resource;

		std::shared_ptr<RHIDescriptorSetLayout> m_BaseDataDescriptorSetLayout; // for view matrices, etc.
		std::shared_ptr<RHIDescriptorSetLayout> m_SceneDataDescriptorSetLayout; // for scene data descriptor set layout
		std::shared_ptr<RHIDescriptorSetLayout> m_PbrDescriptorSetLayout; // for pbr descriptor set layout
		std::vector<std::shared_ptr<RHIDescriptorSet>> m_BaseDataDescriptorSet;
		std::vector<std::shared_ptr<RHIDescriptorSet>> m_SceneDataDescriptorSet;
		std::vector<std::shared_ptr<RHIBuffer>> m_BaseDataUniformBuffer;
		std::vector<std::shared_ptr<RHIBuffer>> m_SceneDataUniformBuffer;
		std::vector<std::shared_ptr<RHIBuffer>> m_PerObjectBuffer;

		RenderContext* m_RenderContext; // Context for rendering operations
		std::unique_ptr<UIPass> m_UIPass; // UI rendering pass
		std::unique_ptr<SkyboxPass> m_SkyboxPass; // Skybox rendering pass
		std::unique_ptr<PbrPass> m_PbrPass; // PBR rendering pass
		std::unique_ptr<CullingPass> m_CullingPass; // GPU frustum culling pass
		std::unique_ptr<HiZPass> m_HiZPass; // Hi-Z pyramid build for occlusion culling
		std::unique_ptr<ShadowPass> m_ShadowPass; // cascaded shadow maps
		uint32_t m_ShadowDebug = 0; // 0 = off, 1 = shadow factor, 2 = cascade colors, 3 = raw map
		// world-space bounds of all submitted casters, consumed by ShadowPass to size its depth range
		glm::vec3 m_CasterBoundsMin{ 0.0f };
		glm::vec3 m_CasterBoundsMax{ 0.0f };
		RenderGraph m_RenderGraph; // frame graph: ordering + barrier planning
		bool m_DumpGraphRequested = false;
		std::unique_ptr<OitPass> m_OitPass; // Order Independent Transparency pass

		bool OitEnabled = false; // Toggle for Order Independent Transparency
		bool m_TextureCompressionEnabled = false; // Toggle for neural texture compression

		RenderStats m_FrameStats;

		std::shared_ptr<Texture> m_IBLTexture = nullptr;
		std::shared_ptr<RHIDescriptorSet> m_IBLDescriptorSet = nullptr;
	};
}
