#include "bearpch.h"

#include "SceneLayer.h"
#include "Application.h"
#include "AssetLoader.h"
#include "Scene.h"
#include "RenderObject.h"
#include "Core/ResourceManager.h"
#include "BaseData.h"
namespace Bear
{
	SceneLayer::SceneLayer(std::unique_ptr<Scene> scene)
		:Layer("SceneLayer"), m_Scene(std::move(scene))
	{
	}

	SceneLayer::~SceneLayer()
	{
	}
	
	void SceneLayer::OnAttach()
	{
		// 1. load gltf scene, file --> cpu
		// auto modelDesc = AssetLoader::ImportModel("assets/models/Sponza/glTF/Sponza.gltf");
		// auto modelDesc = AssetLoader::ImportModel("assets/models/New_FlightHelmet/FlightHelmet.gltf");
		// auto modelDesc = AssetLoader::ImportModel("assets/models/DamagedHelmet/glTF/DamagedHelmet.gltf");
		auto modelDesc = AssetLoader::ImportModel("assets/models/TransmissionOrderTest/glTF/TransmissionOrderTest.gltf");
		// auto modelDesc = AssetLoader::ImportModel("assets/models/TransmissionTest/glTF/TransmissionTest.gltf");
		// auto modelDesc = AssetLoader::ImportModel("assets/models/SciFiHelmet/glTF/SciFiHelmet.gltf");
		// auto modelDesc = AssetLoader::ImportModel("assets/models/Suzanne/glTF/Suzanne.gltf");
		// 2. resource system create descriptor infos, cpu --> gpu
		auto& app = Application::Get();
		auto& resourceManager = app.GetRenderer()->GetResourceManager();
		auto resources = resourceManager.CreateResources(modelDesc);

		// 3. create scene graph
		m_Scene->CreateSceneGraph(modelDesc, resources);
	}
	void SceneLayer::OnDetach()
	{
	}
	void SceneLayer::OnUpdate(float deltaTime)
	{
		m_Scene->Update(deltaTime);
	}
	void SceneLayer::OnRender() const
	{
		// 1. collect render objects from scene graph
		std::vector<RenderObject> renderObjects;
		SceneData sceneData;
		BaseData baseData;
		m_Scene->CollectRenderData(renderObjects, sceneData, baseData);
		// 2. submit render objects
		auto& app = Application::Get();
		auto renderer = app.GetRenderer();
		renderer->Submit(renderObjects, sceneData, baseData);
	}
	void SceneLayer::OnEvent(Event& event)
	{
		m_Scene->OnEvent(event);
	}
	bool SceneLayer::OnKeyPress(Event& event)
	{
		// TODO: implement key press handling
		return false;
	}
}
