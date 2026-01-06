#include <Bear.h>


class SandboxApp : public Bear::Application
{
public:
	SandboxApp()
	{
		auto scene = std::make_unique<Bear::Scene>();
		PushLayer(new Bear::SceneLayer(std::move(scene)));
		PushOverlay(new Bear::GuiLayer());
	}
	~SandboxApp()
	{
	}
};

Bear::Application* Bear::CreateApplication()
{
	return new SandboxApp();
}