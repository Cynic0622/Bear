#pragma once
#include <memory>
#include <vector>

#include "Mesh.h"
#include "Texture.h"

namespace Bear
{
	class Resource;
	struct SceneData;
	class Scene;
	class LayerStack;
	class Material;
}

namespace Bear
{
	class RHIPipelineLayout;
}

struct GLFWwindow;
namespace Bear {

	enum class GraphicsAPI;
	class RHIDevice;
	class RHISwapchain;
	class RHIRenderPass;
	class RenderObject;

	struct globalParams
	{
		glm::mat4 viewMatrix;
		glm::mat4 projectionMatrix;
	};

	class Renderer {
	public:
		Renderer(GLFWwindow* window, GraphicsAPI api);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer&& operator=(Renderer&&) = delete;

		void OnWindowResized() const;

		// getter
		RHIDevice* GetDevice() const { return m_Device.get(); }
		RHIRenderPass* GetRenderPass() const { return m_RenderPass.get(); }
		TextureManager* GetTextureManager() const { return m_TextureManager.get(); }
		MeshManager* GetMeshManager() const { return m_MeshManager.get(); }
		RHICommandList* GetCurrentCommandList() const { return m_CurrentCommandBuffer; }
		uint32_t GetSwapchainImageCount() const;
		Resource& GetResource() { return *m_Resource; }

		void BeginFrame();
		void Submit(const std::vector<RenderObject>& renderObjects, const SceneData& sceneData);
		void EndFrame() const;


	private:
		void Init(GraphicsAPI api);
		void LoadResources();

	private:
		GLFWwindow* m_Window;
		const uint8_t MAX_FRAMES_IN_FLIGHT = 2;
		std::unique_ptr<RHIDevice> m_Device;
		std::unique_ptr<RHISwapchain> m_Swapchain;
		std::shared_ptr<RHIRenderPass> m_RenderPass;
		std::shared_ptr<RHIPipelineLayout> m_PipelineLayout; // for pipelines
		std::shared_ptr<RHIPipeline> m_Pipeline; // for rendering

		std::unique_ptr<TextureManager> m_TextureManager;
		std::unique_ptr<MeshManager> m_MeshManager;

		RHICommandList* m_CurrentCommandBuffer;
		uint32_t m_CurrentImageIndex;
		std::unique_ptr<Resource> m_Resource;

		std::vector<std::shared_ptr<RHIDescriptorSetLayout>> m_GlobalDescriptorSetLayout; // for view matrices, lights, etc.
		std::vector<std::shared_ptr<RHIDescriptorSet>> m_GlobalDescriptorSet;
		std::vector<std::shared_ptr<RHIBuffer>> m_GlobalUniformBuffer;
		globalParams m_GlobalParams;
	};
}