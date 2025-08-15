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
namespace Bear
{
	SceneLayer::SceneLayer(std::unique_ptr<Scene> scene)
		:Layer("SceneLayer"), m_Scene(std::move(scene))
	{
		m_EditorCamera = std::make_unique<EditorCamera>(45.0f, (float)1280/720, 1.f, 1000.f);
	}

	SceneLayer::~SceneLayer()
	{
	}
	
	void SceneLayer::OnAttach()
	{
		// 1. load gltf scene
		auto modelDesc = AssetLoader::ImportModel("assets/models/Sponza/glTF/Sponza.gltf");
		// 2. resource system create descriptor infos

		// 3. create descriptor sets

		// 4. create scene graph
	}
	void SceneLayer::OnDetach()
	{
	}
	void SceneLayer::OnUpdate(float deltaTime)
	{
		m_EditorCamera->Update(deltaTime);
		m_Scene->Update();
		m_SceneData = { m_EditorCamera->GetViewMatrix(), m_EditorCamera->GetProjectionMatrix() };
	}
	void SceneLayer::OnRender() const
	{
		// 1. collect render objects from scene graph

		// 2. sort render objects by material

		// 3. bind descriptor sets and pipeline

		// 4. draw render objects
	}
	void SceneLayer::OnEvent(Event& event)
	{
		m_EditorCamera->OnEvent(event);
	}
}
