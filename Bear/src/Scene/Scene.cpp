#include "bearpch.h"
#include "Scene.h"
#include "SceneLayer.h"
#include "Application.h"
#include "AssetLoader.h"
#include "Component.h"
#include "RenderObject.h"
#include "Entity.h"
#include "Resource.h"
#include "CameraController.h"
namespace Bear
{
	Scene::Scene()
	{
		m_GameCamera = std::make_unique<CameraController>(45.0f, (float)1280 / 720, 0.1f, 1000.f);
		m_EditorCamera = std::make_unique<CameraController>(45.0f, (float)1280 / 720, 0.1f, 1000.f);
		// add some random lights for sponza scene.
		std::srand(static_cast<unsigned>(std::time(nullptr)));
		for (int i = 0; i < 1; i++)
		{
			Entity light = CreateEntity("Light" + std::to_string(i));
			auto& transform = light.GetComponent<TransformComponent>();
			transform.Position = glm::vec3(static_cast<float>(std::rand() % 30 - 15),
				static_cast<float>(std::rand() % 30),
				static_cast<float>(std::rand() % 30 - 15));
			glm::vec3 color = glm::vec3(1.0f);
			// light.AddComponent<LightComponent>(glm::vec3(color),10);
			light.AddComponent<LightComponent>(LightComponent::CreatePoint(glm::vec4(color, 10)));
		}
		Entity light = CreateEntity("Directional Light");
		light.AddComponent<LightComponent>(LightComponent::CreateDirectional({1.f, -1.f, 1.f}, {1.f, 1.f, 1.f, 10.f}));

	}
	Scene::~Scene()
	{
	}
	void Scene::Update(float deltaTime)
	{
		auto view = m_Registry.view<TransformComponent, HierarchyComponent>();
		for (auto entity : view)
		{
			/*Entity e = { entity, this };*/
			auto& hierarchy = m_Registry.get<HierarchyComponent>(entity);
			if (hierarchy.Parent != entt::null)
			{
				auto& parentTransform = m_Registry.get<TransformComponent>(hierarchy.Parent);
				auto& transform = m_Registry.get<TransformComponent>(entity);
				transform.WorldTransform = parentTransform.GetWorldTransform() * transform.GetLocalTransform();
			}
			else
			{
				auto& transform = m_Registry.get<TransformComponent>(entity);
				transform.WorldTransform = transform.GetLocalTransform();
			}
		}
		// update light position
		auto lightView = m_Registry.view<TransformComponent, LightComponent>();
		for (auto entity : lightView)
		{
			auto& transform = m_Registry.get<TransformComponent>(entity);
			glm::vec3 rotate = glm::rotate(glm::mat4(1.0f), glm::radians((float)glfwGetTime() / 2), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(transform.Position, 1.0f);
			// transform.Position = rotate;
			transform.WorldTransform = transform.GetLocalTransform();
		}
		GetActiveCamera()->Update(deltaTime);
	}
	void Scene::CollectRenderObjects(std::vector<RenderObject>& ObjectsList, SceneData& sceneData)
	{
		auto view = m_Registry.view<MeshComponent, TransformComponent, MaterialComponent>();
		for (auto entity : view)
		{
			auto& meshComponent = m_Registry.get<MeshComponent>(entity);
			auto& transformComponent = m_Registry.get<TransformComponent>(entity);
			RenderObject renderObject;
			renderObject.mesh = meshComponent.MeshRes;
			renderObject.transform = transformComponent.GetWorldTransform();
			renderObject.material = m_Registry.try_get<MaterialComponent>(entity) ? m_Registry.get<MaterialComponent>(entity).MaterialRes : nullptr;
			// frustum culling check.
			if (m_FrustumCull && !GetActiveCamera()->GetFrustum().Contains(renderObject.GetAABB()))
				continue;
			ObjectsList.push_back(renderObject);
		}
		// BEAR_CORE_INFO("FrustumCull state: {}, RenderObject size: {}", m_FrustumCull, ObjectsList.size());
		// collect scene data
		auto lightView = m_Registry.view<TransformComponent, LightComponent>();
		uint8_t index = 0;
		for (auto entity : lightView)
		{
			auto& transformComponent = m_Registry.get<TransformComponent>(entity);
			auto& lightComponent = m_Registry.get<LightComponent>(entity);

			switch (lightComponent.Type)
			{
			case LightType::Point:
				{
					sceneData.pointLight[index].position = glm::vec4(transformComponent.Position, 1.0f);
					sceneData.pointLight[index].color = glm::vec4(lightComponent.Color);
					index++;
					break;
				}
			default:
				break;
			}
		}
		// camera data
		sceneData.cameraPosition = glm::vec4(GetActiveCamera()->GetPosition(), 1.f);
		sceneData.viewMatrix = GetActiveCamera()->GetViewMatrix();
		sceneData.projectionMatrix = GetActiveCamera()->GetProjectionMatrix();
	}
	Entity Scene::CreateEntity(const std::string& name)
	{
		Entity entity = { m_Registry.create(), this };
		entity.AddComponent<IDComponent>();
		entity.AddComponent<TagComponent>(name);
		entity.AddComponent<TransformComponent>();
		entity.AddComponent<HierarchyComponent>();
		return entity;
	}
	void Scene::DestroyEntity(Entity entity)
	{
		m_Registry.destroy(entity);
	}
	void Scene::CreateSceneGraph(const ModelDescription& desc, const Resources& resources)
	{
		// create scene graph from scene roots, bind the node description to the entity, such as name, transform, mesh, material, etc.
		std::function<void(uint32_t, Entity)> buildScene = [&](uint32_t index, Entity parentEntity)
		{
			const auto& nodeDesc = desc.nodes[index];
			Entity entity = CreateEntity(nodeDesc.name);
			auto& transform = m_Registry.get<TransformComponent>(entity);
			transform = TransformComponent(nodeDesc.translation, nodeDesc.scale, nodeDesc.rotation);

			if (nodeDesc.meshIndex >= 0 && nodeDesc.meshIndex < desc.meshes.size())
			{
				const auto& mesh = desc.meshes[nodeDesc.meshIndex];
				for (size_t i = mesh.firstPrimitiveIndex; i < mesh.primitiveCount + mesh.firstPrimitiveIndex; i++)
				{
					const auto& primitiveDesc = desc.primitives[i];
					auto primitiveMesh = resources.Meshes[i];

					Entity child = CreateEntity();
					child.AddComponent<MeshComponent>(primitiveMesh);
					auto& hierarchy = child.GetComponent<HierarchyComponent>();
					hierarchy.Parent = entity;
					if (primitiveDesc.materialIndex >= 0 && primitiveDesc.materialIndex < desc.materials.size())
					{
						auto material = resources.Materials[primitiveDesc.materialIndex];
						child.AddComponent<MaterialComponent>(material);
					}
				}
			}

			if (parentEntity)
			{
				auto& hierarchy = entity.GetComponent<HierarchyComponent>();
				hierarchy.Parent = parentEntity;
			}
			for (const auto& childIndex : nodeDesc.childrenIndices)
			{
				buildScene(childIndex, entity);
			}
		};

		for (auto& root : desc.rootNodeIndices)
		{
			buildScene(root, CreateEntity()); // root entity has no parent
		}
	}

	void Scene::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& event) { return this->OnKeyPress(event); });
		if (event.IsHandled()) return;
		GetActiveCamera()->OnEvent(event);
	}

	bool Scene::OnKeyPress(Event& event)
	{
		if (Input::IsKeyPressed(Key::Z))
		{
			m_FrustumCull = !m_FrustumCull;
			BEAR_CORE_TRACE("The state of the FrustumCull : {}", m_FrustumCull);
			return true;
		}
		if (Input::IsKeyPressed(Key::Q))
		{
			m_EditorMode = !m_EditorMode;
			BEAR_CORE_TRACE("The state of the EditorMode : {}", m_EditorMode);
			return true;
		}
		return false;
	}

	CameraController* Scene::GetActiveCamera() const
	{
		return m_EditorMode ? m_EditorCamera.get() : m_GameCamera.get();
	}
}