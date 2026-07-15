#include <Bear.h>
#include "Scene/SceneLayer.h"
#include "Bear/Gui/GuiLayer.h"

class SandboxApp : public Bear::Application
{
public:
	SandboxApp()
	{
		auto scene = std::make_unique<Bear::Scene>();
		auto* sceneLayer = new Bear::SceneLayer(std::move(scene));
		auto* guiLayer = new Bear::GuiLayer();

		guiLayer->SetOnModelSwitchCallback([sceneLayer](const std::string& path) {
			sceneLayer->LoadModel(path);
		});
		guiLayer->SetOnReloadModelCallback([sceneLayer]() {
			sceneLayer->ReloadModel();
		});
		guiLayer->SetOnInstanceCountChangeCallback([sceneLayer](uint32_t count) {
			sceneLayer->SetInstanceMultiplier(count);
		});

		PushLayer(sceneLayer);
		PushOverlay(guiLayer);
	}
	~SandboxApp()
	{
	}
};

Bear::Application* Bear::CreateApplication()
{
	return new SandboxApp();
}