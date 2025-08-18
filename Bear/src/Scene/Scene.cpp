#include "bearpch.h"
#include "Node.h"
#include "Scene.h"
#include "SceneLayer.h"
#include "Application.h"
#include "AssetLoader.h"
#include "Component.h"
#include "RenderObject.h"
#include "Entity.h"
#include "Resource.h"
#include "Common/Mesh.h"
#include "CameraController.h"
namespace Bear
{
	Scene::Scene()
	{
		m_Camera = std::make_unique<CameraController>(45.0f, (float)1280 / 720, 0.1f, 10000.f);

		// add some random lights for sponza scene.
		std::srand(static_cast<unsigned>(std::time(nullptr)));
		for (int i = 0; i < 50; i++)
		{
			Entity light = CreateEntity("Light" + std::to_string(i));
			auto& transform = light.GetComponent<TransformComponent>();
			transform.Position = glm::vec3(static_cast<float>(std::rand() % 1000 - 500),
				static_cast<float>(std::rand() % 1000),
				static_cast<float>(std::rand() % 1000 - 500));
			light.AddComponent<LightComponent>(glm::vec3(static_cast<float>(std::rand() % 100) / 99.0f,
				static_cast<float>(std::rand() % 100) / 99.0f,
				static_cast<float>(std::rand() % 100) / 99.0f), std::rand() % 50000);
		}
	}
	Scene::~Scene()
	{
	}
	void Scene::Update(bool editorMode, float deltaTime)
	{
		/*glm::quat newRot = glm::angleAxis((float)glfwGetTime(), glm::vec3(0, 0, 1));
		m_RootNode->SetRotation(newRot);
		m_RootNode->Update();*/
		auto view = m_Registry.view<TransformComponent, HierarchyComponent>();
		for (auto entity : view)
		{
			/*Entity e = { entity, this };*/
			auto& hierarchy = m_Registry.get<HierarchyComponent>(entity);
			if (hierarchy.Parent != entt::null)
			{
				auto& parentTransform = m_Registry.get<TransformComponent>(hierarchy.Parent);
				auto& transform = m_Registry.get<TransformComponent>(entity);
				transform.Transform = parentTransform.Transform * transform.GetTransform();
			}
			else
			{
				auto& transform = m_Registry.get<TransformComponent>(entity);
				transform.Transform = transform.GetTransform();
			}
		}
		if (!editorMode)
		{
			m_Camera->Update(deltaTime);
		}
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
			renderObject.transform = transformComponent.GetTransform();
			renderObject.material = m_Registry.try_get<MaterialComponent>(entity) ? m_Registry.get<MaterialComponent>(entity).MaterialRes : nullptr;
			ObjectsList.push_back(renderObject);
		}
		// collect scene data
		auto lightView = m_Registry.view<TransformComponent, LightComponent>();
		uint8_t index = 0;
		for (auto entity : lightView)
		{
			auto& transformComponent = m_Registry.get<TransformComponent>(entity);
			auto& lightComponent = m_Registry.get<LightComponent>(entity);
			sceneData.lightsData[index].position = glm::vec4(transformComponent.Position, 1.0f);
			sceneData.lightsData[index].colorIntensity = glm::vec4(lightComponent.Color, lightComponent.Intensity);
			index++;
		}
		// camera data
		sceneData.cameraPosition = glm::vec4(m_Camera->GetPosition(), 1.f);
		sceneData.viewMatrix = m_Camera->GetViewMatrix();
		sceneData.projectionMatrix = m_Camera->GetProjectionMatrix();
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
				for (size_t i = mesh.firstPrimitiveIndex; i < mesh.primitiveCount; i++)
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
		m_Camera->OnEvent(event);
	}

	void Scene::CollectRenderObjectsRecursive(std::vector<RenderObject>& renderList)
	{
		
	}
	void Scene::InstantiateModel(const ModelDescription& description)
	{
		// instanced the textures
		std::vector<std::shared_ptr<Texture>> textures;
		/*for (const auto& imgDesc : description.images)
		{
			auto texture = m_TextureManager->Load(imgDesc.filepath, *Application::Get().GetRenderer()->GetDevice(), 
				imgDesc.width, imgDesc.height, imgDesc.pixels.data());
			textures.push_back(std::move(texture));
		}*/

		// instanced the materials
		std::vector<std::shared_ptr<Material>> materials;
		/*for (const auto& matDesc : description.materials)
		{
			auto material = m_MaterialManager->Load(matDesc.name, matDesc.shaderName, textures);
			materials.push_back(std::move(material));
		}*/
	}
}