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
	static const std::vector<std::pair<const char*, const char*>> s_ModelList = {
		{"Sponza",                    "assets/models/Sponza/glTF/Sponza.gltf"},
		{"DamagedHelmet",             "assets/models/DamagedHelmet/glTF/DamagedHelmet.gltf"},
		{"FlightHelmet",              "assets/models/FlightHelmet/FlightHelmet.gltf"},
		{"New_FlightHelmet",          "assets/models/New_FlightHelmet/FlightHelmet.gltf"},
		{"SciFiHelmet",               "assets/models/SciFiHelmet/glTF/SciFiHelmet.gltf"},
		{"Suzanne",                   "assets/models/Suzanne/glTF/Suzanne.gltf"},
		{"TransmissionOrderTest",     "assets/models/TransmissionOrderTest/glTF/TransmissionOrderTest.gltf"},
		{"TransmissionTest",           "assets/models/TransmissionTest/glTF/TransmissionTest.gltf"},
		{"CesiumMan",                 "assets/models/CesiumMan/CesiumMan.gltf"},
	};

	SceneLayer::SceneLayer(std::unique_ptr<Scene> scene)
		:Layer("SceneLayer"), m_Scene(std::move(scene))
	{
	}

	SceneLayer::~SceneLayer()
	{
	}

	const std::vector<std::pair<const char*, const char*>>& SceneLayer::GetModelList()
	{
		return s_ModelList;
	}
	
	void SceneLayer::OnAttach()
	{
		LoadModel(s_ModelList[0].second);
	}

	void SceneLayer::LoadModel(const std::string& path)
	{
		BEAR_CORE_INFO("Loading model: {}", path);
		auto& app = Application::Get();
		auto& resourceManager = app.GetRenderer()->GetResourceManager();

		auto modelDesc = AssetLoader::ImportModel(path);
		auto resources = resourceManager.CreateResources(modelDesc);

		m_Scene->ClearAllEntities();
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
