#include "bearpch.h"

#include "SceneLayer.h"
#include "Application.h"
#include "Node.h"
#include "Scene.h"
#include "RenderObject.h"
#include "Scene/EditorCamera.h"
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
		auto node1 = std::make_unique<Node>();
		auto node2 = std::make_unique<Node>();
		auto node3 = std::make_unique<Node>();
		auto& app = Application::Get();
		auto renderer = app.GetRenderer();
		auto device = renderer->GetDevice();
		auto renderPass = renderer->GetRenderPass();
		auto mesh = renderer->GetMeshManager()->Load("../../../models/planet/planet.obj", *device, "../../../models/planet/planet.obj");
		auto tex = renderer->GetTextureManager()->Load("../../../models/planet/mars.png", *device, "../../../models/planet/mars.png");
		std::vector<std::string> shaderPaths = {
			"../../../Bear/src/Bear/Shaders/tri.vert.spv",
			"../../../Bear/src/Bear/Shaders/tri.frag.spv"
		};
		auto material = std::make_shared<Material>(*device, *renderPass, shaderPaths);
		material->SetTexture(1, tex);
		node1->SetMesh(mesh);
		node1->SetMaterial(material);


		node2->SetMesh(mesh);
		node2->SetMaterial(material);
		node2->SetPosition(glm::vec3(8.f, 0.f, 0.f));
		node2->SetScale(glm::vec3(1.f));

		node3->SetMesh(mesh);
		node3->SetMaterial(material);
		node3->SetPosition(glm::vec3(5.f, 0.f, 0.f));
		node3->SetScale(glm::vec3(0.5f));
		auto rootNode = m_Scene->GetRootNode();
		node2->AddChild(std::move(node3));
		node1->AddChild(std::move(node2));
		rootNode->AddChild(std::move(node1));
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
		auto& app = Application::Get();
		auto renderer = app.GetRenderer();
		std::vector<RenderObject> renderObjects;
		m_Scene->CollectRenderObjects(renderObjects);
		
		renderer->Submit(renderObjects, m_SceneData);
	}
	void SceneLayer::OnEvent(Event& event)
	{
		m_EditorCamera->OnEvent(event);
	}
}
