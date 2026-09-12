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
		guiLayer->SetDrawStatsProvider([sceneLayer]() -> Bear::RenderStats {
			auto stats = Bear::Application::Get().GetRenderer()->GetFrameStats();
			stats.sceneObjects = sceneLayer->GetTotalMeshEntities();
			return stats;
		});
		guiLayer->SetRenderStatesProvider([sceneLayer]() -> Bear::RenderStates {
			Bear::RenderStates states;
			states.frustumCull = sceneLayer->IsFrustumCullingEnabled();
			states.gpuCulling = Bear::Application::Get().GetRenderer()->IsGpuCullingEnabled();
			states.oitEnabled = Bear::Application::Get().GetRenderer()->IsOitEnabled();
			states.editorMode = sceneLayer->IsEditorMode();
			return states;
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