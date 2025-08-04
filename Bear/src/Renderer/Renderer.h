#pragma once
#include <memory>
#include <vector>

namespace Bear
{
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

		void DrawFrame(LayerStack& layerStack);
		void OnWindowResized();

		inline RHIDevice* GetDevice() const { return m_Device.get(); }
		inline RHIRenderPass* GetRenderPass() const { return m_RenderPass.get(); }

		uint32_t GetSwapchainImageCount() const;

	private:
		void Init(GraphicsAPI api);
		void LoadResources();

	private:
		GLFWwindow* m_Window;

		std::unique_ptr<RHIDevice> m_Device;
		std::unique_ptr<RHISwapchain> m_Swapchain;
		std::shared_ptr<RHIRenderPass> m_RenderPass;
		std::shared_ptr<RHIPipelineLayout> m_PipelineLayout;
		std::shared_ptr<class Mesh> m_SquareMesh;
		std::shared_ptr<Material> m_SimpleMaterial;

		std::shared_ptr<class Mesh> m_Mesh;
		std::shared_ptr<class Material> m_Material;
		std::vector<std::unique_ptr<RenderObject>> m_RenderObjects;
	};
}