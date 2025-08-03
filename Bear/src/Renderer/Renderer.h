#pragma once
#include <memory>
#include <vector>
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

		void DrawFrame();
		void OnWindowResized();

	private:
		void Init(GraphicsAPI api);
		void LoadResources();

	private:
		GLFWwindow* m_Window;

		std::unique_ptr<RHIDevice> m_Device;
		std::unique_ptr<RHISwapchain> m_Swapchain;
		std::shared_ptr<RHIRenderPass> m_RenderPass;

		std::shared_ptr<class Mesh> m_Mesh;
		std::shared_ptr<class Material> m_Material;
		std::vector<std::unique_ptr<RenderObject>> m_RenderObjects;
	};
}