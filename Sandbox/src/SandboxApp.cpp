#include <Bear.h>



class ExampleLayer : public Bear::Layer
{
	public:
	ExampleLayer()
			: Layer("ExampleLayer")
	{
	}
	void OnUpdate(float deltaTime) override
	{
		BEAR_CLIENT_INFO("{0} is updating!", GetName());
	}
	void OnEvent(Bear::Event& event) override
	{
		BEAR_CLIENT_INFO("Event received in ExampleLayer: {0}", event);
	}
};
class SandboxApp : public Bear::Application
{
	public:
	SandboxApp()
	{
		//PushLayer(new ExampleLayer());
		//PushOverlay(new Bear::GuiLayer());
		//PushLayer(new Bear::GuiLayer());
		auto scene = std::make_unique<Bear::Scene>();
		PushLayer(new Bear::SceneLayer(std::move(scene)));
		PushOverlay(new Bear::VulkanGuiLayer());
	}
	~SandboxApp()
	{
	}
};

Bear::Application* Bear::CreateApplication()
{
	return new SandboxApp();
}