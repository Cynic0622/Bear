#include "bearpch.h"
#include "Scene.h"
#include "Application.h"
#include "AssetLoader.h"
#include "Component.h"
#include "RenderObject.h"
#include "Entity.h"
#include "ResourceManager.h"
#include "CameraController.h"
#include "BaseData.h"
#include <random>
namespace Bear
{
	Scene::Scene()
	{
		m_GameCamera = std::make_unique<CameraController>(45.0f, (float)1280 / 720, 0.1f, 1000.f);
		m_EditorCamera = std::make_unique<CameraController>(45.0f, (float)1280 / 720, 0.1f, 1000.f);
		// add some random lights for sponza scene.
		std::srand(static_cast<unsigned>(std::time(nullptr)));
		for (int i = 0; i < m_LightNumber; i++)
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
		light.AddComponent<LightComponent>(LightComponent::CreateDirectional({ -1.0, -2.5, -1.0 }, {1.f, .98f, .9f, 4.f}));

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
	void Scene::CollectRenderData(std::vector<RenderObject>& ObjectsList, SceneData& sceneData, BaseData& baseData)
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
					if (index >= MAX_POINT_LIGHTS)
						break;
					sceneData.pointLights[index].color = lightComponent.Color;
					sceneData.pointLights[index].position = glm::vec4(transformComponent.Position, 1.0f);
					index++;
					break;
				}
				case LightType::Directional:
				{
					sceneData.dirLight.color = lightComponent.Color;
					sceneData.dirLight.direction = glm::vec4(lightComponent.Direction, 0.0f);
					break;
				}
			default:
				break;
			}
			sceneData.numLights = index;
		}
		// camera data
		baseData.cameraPosition = glm::vec4(GetActiveCamera()->GetPosition(), 1.f);
		baseData.viewMat = GetActiveCamera()->GetViewMatrix();
		baseData.projMat = GetActiveCamera()->GetProjectionMatrix();
		baseData.frameIndex = (m_FrameNum++) % UINT32_MAX;
	}
	Entity Scene::CreateEntity(const std::string& name)
	{
		Entity entity = { m_Registry.create(), this };
		entity.AddComponent<IDComponent>(m_NextUID);
		entity.AddComponent<TagComponent>(name);
		entity.AddComponent<TransformComponent>();
		entity.AddComponent<HierarchyComponent>();
		return entity;
	}
	void Scene::DestroyEntity(Entity entity)
	{
		m_Registry.destroy(entity);
	}

	void Scene::AddLight(const PointLight& light)
	{
		Entity pointLight = CreateEntity("Point Light");
		auto& transform = pointLight.GetComponent<TransformComponent>();
		transform.Position = glm::vec3(light.position);
		pointLight.AddComponent<LightComponent>(LightComponent::CreatePoint(light.color));
	}

	void Scene::ClearAllEntities()
	{
		m_Registry.clear();
		Entity light = CreateEntity("Directional Light");
		light.AddComponent<LightComponent>(LightComponent::CreateDirectional({ -1.0, -2.5, -1.0 }, {1.f, .98f, .9f, 4.f}));
	}

	void Scene::MultiplyInstances(uint32_t count, float spread)
	{
		if (count <= 1) return;
		auto view = m_Registry.view<MeshComponent, MaterialComponent, TransformComponent>();
		std::vector<std::tuple<MeshComponent, MaterialComponent, TransformComponent>> templates;
		for (auto entity : view)
		{
			templates.emplace_back(
				m_Registry.get<MeshComponent>(entity),
				m_Registry.get<MaterialComponent>(entity),
				m_Registry.get<TransformComponent>(entity));
		}

		std::mt19937 rng{ std::random_device{}() };
		std::uniform_real_distribution<float> posDist(-spread, spread);
		std::uniform_real_distribution<float> rotDist(0.0f, 360.0f);

		for (uint32_t i = 1; i < count; ++i)
		{
			for (const auto& [mesh, mat, tf] : templates)
			{
				Entity clone = CreateEntity("Instance_" + std::to_string(i));
				clone.AddComponent<MeshComponent>(mesh.MeshRes);
				clone.AddComponent<MaterialComponent>(mat.MaterialRes);
				auto& cloneTf = clone.GetComponent<TransformComponent>();
				cloneTf.Position = tf.Position + glm::vec3(posDist(rng), posDist(rng) * 0.3f, posDist(rng));
				cloneTf.Scale = tf.Scale;
				cloneTf.Rotation = glm::vec3(0.0f, rotDist(rng), 0.0f);
			}
		}
		BEAR_CORE_INFO("Multiplied instances: {} → {} (x{})", (uint32_t)templates.size(), (uint32_t)(templates.size() * count), count);
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