#include "bearpch.h"

#include "SceneLayer.h"
#include "Application.h"
#include "AssetLoader.h"
#include "Component.h"
#include "Node.h"
#include "Scene.h"
#include "RenderObject.h"
#include "Scene/CameraController.h"
#include "Entity.h"
#include "Core/Resource.h"
namespace Bear
{
	SceneLayer::SceneLayer(std::unique_ptr<Scene> scene)
		:Layer("SceneLayer"), m_Scene(std::move(scene))
	{
		m_EditorCamera = std::make_unique<CameraController>(45.0f, (float)1280/720, 0.1f, 10000.f);
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
		if (m_EditorMode)
		{
			m_EditorCamera->Update(deltaTime);
		}
		m_Scene->Update(m_EditorMode, deltaTime);
	}
	void SceneLayer::OnRender() const
	{
		// 1. collect render objects from scene graph
		std::vector<RenderObject> renderObjects;
		SceneData sceneData;
		m_Scene->CollectRenderObjects(renderObjects, sceneData);
		if (m_EditorMode)
		{
			sceneData.cameraPosition = glm::vec4(m_EditorCamera->GetPosition(), 1.f);
			sceneData.viewMatrix = m_EditorCamera->GetViewMatrix();
			sceneData.projectionMatrix = m_EditorCamera->GetProjectionMatrix();
		}
		// 2. submit render objects
		auto& app = Application::Get();
		auto renderer = app.GetRenderer();
		renderer->Submit(renderObjects, sceneData);
	}
	void SceneLayer::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& e) { return this->OnKeyPress(e); });
		if (event.IsHandled()) return;
		if (m_EditorMode)
		{
			m_EditorCamera->OnEvent(event);
		}
		if (!event.IsHandled())
		m_Scene->OnEvent(event);
	}
	bool SceneLayer::OnKeyPress(Event& event)
	{
		if (Input::IsKeyPressed(Key::Q))
		{
			m_EditorMode = !m_EditorMode; // toggle editor mode
			return true;
		}
		return false;
	}
}
