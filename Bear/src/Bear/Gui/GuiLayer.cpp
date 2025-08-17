#include "bearpch.h"
#include "GuiLayer.h"
#include "Bear/Application.h"
#include "Backend/OpenGL/GuiOpenGLRenderer.h"
#include "imgui_impl_glfw.h"
namespace Bear
{
	GuiLayer::GuiLayer()
		: Layer("GuiLayer")
	{
	}

	GuiLayer::~GuiLayer()
	{
	}

	void GuiLayer::OnAttach()
	{
		ImGui::CreateContext();
		ImGui::StyleColorsDark();

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiBackendFlags_HasMouseCursors; // Enable Mouse Cursors
		io.ConfigFlags |= ImGuiBackendFlags_HasSetMousePos; // Enable SetMousePos backend function

		auto& app = Application::Get();
		GLFWwindow* window =  static_cast<GLFWwindow*>(app.GetWindow().GetNativeWindow());
		ImGui_ImplGlfw_InitForOpenGL(window, true);
		io.DisplaySize = ImVec2(static_cast<float>(app.GetWindow().GetWidth()),
			static_cast<float>(app.GetWindow().GetHeight()));

		ImGui_ImplOpenGL3_Init("#version 410");
	}

	void GuiLayer::OnDetach()
	{
	}

	void GuiLayer::OnUpdate(float deltaTime)
	{

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		// Render your GUI here
		/*ImGui::Begin("Hello, World!");
		ImGui::Text("This is a simple GUI layer.");
		ImGui::End();*/
		ImGui::ShowDemoWindow();
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}

	void GuiLayer::OnEvent(Event& event)
	{
	}
}
