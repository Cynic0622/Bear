#pragma once
#include <memory>
#include <vector>

#include "Mesh.h"
#include "Texture.h"
#include "BaseData.h"

namespace Bear
{
	class OitPass;
	class PbrPass;
	class SkyboxPass;
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

	class Renderer {
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
		RHIRenderPass* GetRenderPass() const { return m_RenderPass.get(); }
		RHICommandList* GetCurrentCommandList() const { return m_CurrentCommandBuffer; }
		uint32_t GetSwapchainImageCount() const;
		ResourceManager& GetResourceManager() { return *m_Resource; }

		void BeginFrame();
		void Submit(const std::vector<RenderObject>& renderObjects, const SceneData& sceneData, const BaseData& baseData);
		void EndFrame() const;


	private:
		void Init(GraphicsAPI api);

	private:
		GLFWwindow* m_Window;
		const uint8_t MAX_FRAMES_IN_FLIGHT = 2;
		std::unique_ptr<RHIDevice> m_Device;
		std::unique_ptr<RHISwapchain> m_Swapchain;
		std::shared_ptr<RHIRenderPass> m_RenderPass;
		std::shared_ptr<RHIRenderPass> m_PreZRenderPass; // for pre-z rendering
		std::shared_ptr<RHIRenderPass> m_UIRenderPass; // for UI rendering
		std::shared_ptr<RHIPipelineLayout> m_PipelineLayout; // for pipelines
		std::shared_ptr<RHIPipelineLayout> m_PreZPipelineLayout;
		std::shared_ptr<RHIPipeline> m_PbrPipeline; // for pbr rendering
		std::shared_ptr<RHIPipeline> m_PreZPipeline;

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

		RenderContext* m_RenderContext; // Context for rendering operations
		std::unique_ptr<UIPass> m_UIPass; // UI rendering pass
		std::unique_ptr<SkyboxPass> m_SkyboxPass; // Skybox rendering pass
		std::unique_ptr<PbrPass> m_PbrPass; // PBR rendering pass
		std::unique_ptr<OitPass> m_OitPass; // Order Independent Transparency pass

		bool OitEnabled = false; // Toggle for Order Independent Transparency

		std::shared_ptr<Texture> m_IBLTexture = nullptr;
		std::shared_ptr<RHIDescriptorSet> m_IBLDescriptorSet = nullptr;
	};
}