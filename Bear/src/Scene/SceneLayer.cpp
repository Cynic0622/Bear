#include "bearpch.h"

#include "SceneLayer.h"
#include "Application.h"
#include "AssetLoader.h"
#include "Component.h"
#include "Node.h"
#include "Scene.h"
#include "RenderObject.h"
#include "Scene/EditorCamera.h"
#include "Entity.h"
#include "Core/Resource.h"
namespace Bear
{
	SceneLayer::SceneLayer(std::unique_ptr<Scene> scene)
		:Layer("SceneLayer"), m_Scene(std::move(scene))
	{
		m_EditorCamera = std::make_unique<EditorCamera>(45.0f, (float)1280/720, 0.1f, 10000.f);
	}

	SceneLayer::~SceneLayer()
	{
	}
	
	void SceneLayer::OnAttach()
	{
		// 1. load gltf scene, file --> cpu
		auto modelDesc = AssetLoader::ImportModel("assets/models/Sponza/glTF/Sponza.gltf");
		// auto modelDesc = AssetLoader::ImportModel("assets/models/Box/glTF/Box.gltf");
		// auto modelDesc = AssetLoader::ImportModel("assets/models/Box with Spaces/glTF/Box with Spaces.gltf");
		// auto modelDesc = AssetLoader::ImportModel("assets/models/ClearCoatTest/glTF/ClearCoatTest.gltf");
		// auto modelDesc1 = AssetLoader::ImportModel("assets/models/Duck/glTF/Duck.gltf");
		// auto modelDesc = AssetLoader::ImportModel("assets/models/Suzanne/glTF/Suzanne.gltf");
		// 2. resource system create descriptor infos, cpu --> gpu
		auto& app = Application::Get();
		auto& renderer = app.GetRenderer()->GetResource();
		auto resources = renderer.CreateResources(modelDesc);

		// 3. create scene graph
		m_Scene->CreateSceneGraph(modelDesc, resources);

		// resources = renderer.CreateResources(modelDesc1);
		// m_Scene->CreateSceneGraph(modelDesc1, resources);
	}
	void SceneLayer::OnDetach()
	{
	}
	void SceneLayer::OnUpdate(float deltaTime)
	{
		m_EditorCamera->Update(deltaTime);
		m_Scene->Update();
		m_SceneData = { m_EditorCamera->GetPosition(), m_EditorCamera->GetViewMatrix(), m_EditorCamera->GetProjectionMatrix() };
	}
	void SceneLayer::OnRender() const
	{
		// 1. collect render objects from scene graph
		std::vector<RenderObject> renderObjects;
		m_Scene->CollectRenderObjects(renderObjects);
		
		// 2. submit render objects
		//m_SceneData = { m_EditorCamera->GetViewMatrix(), m_EditorCamera->GetProjectionMatrix() };
		auto& app = Application::Get();
		auto renderer = app.GetRenderer();
		renderer->Submit(renderObjects, m_SceneData);
	}
	void SceneLayer::OnEvent(Event& event)
	{
		m_EditorCamera->OnEvent(event);
	}
}
