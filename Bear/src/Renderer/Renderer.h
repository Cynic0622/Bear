#pragma once
#include <memory>
#include <vector>

#include "Mesh.h"
#include "Texture.h"

namespace Bear
{
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
		MeshManger* GetMeshManager() const { return m_MeshManager.get(); }
		RHICommandList* GetCurrentCommandList() const { return m_CurrentCommandBuffer; }
		uint32_t GetSwapchainImageCount() const;

		void BeginFrame();
		void Submit(const std::vector<RenderObject>& renderObjects, const SceneData& sceneData) const;
		void EndFrame() const;


	private:
		void Init(GraphicsAPI api);
		void LoadResources();

	private:
		GLFWwindow* m_Window;

		std::unique_ptr<RHIDevice> m_Device;
		std::unique_ptr<RHISwapchain> m_Swapchain;
		std::shared_ptr<RHIRenderPass> m_RenderPass;

		std::unique_ptr<TextureManager> m_TextureManager;
		std::unique_ptr<MeshManger> m_MeshManager;

		RHICommandList* m_CurrentCommandBuffer;
		uint32_t m_CurrentImageIndex;
	};
}