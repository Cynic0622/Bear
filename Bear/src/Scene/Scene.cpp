#include "bearpch.h"
#include "Node.h"
#include "Scene.h"

#include "Application.h"
#include "AssetLoader.h"
#include "Component.h"
#include "RenderObject.h"
#include "Entity.h"
#include "Resource.h"
#include "Common/Mesh.h"
namespace Bear
{
	Scene::Scene()
	{
		m_RootNode = std::make_unique<Node>("rootNode");
		m_MeshManager = std::make_unique<MeshManager>();
		m_MaterialManager = std::make_unique<MaterialManager>();
		m_TextureManager = std::make_unique<TextureManager>();
	}
	void Scene::Update()
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
	}
	void Scene::CollectRenderObjects(std::vector<RenderObject>& ObjectsList)
	{
		if (m_RootNode)
		{
			CollectRenderObjectsRecursive(m_RootNode.get(), ObjectsList);
		}
		else
		{
			BEAR_CORE_ERROR("Scene has no root node!");
		}
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
		// create scene graph from scene roots
		std::function<void(uint32_t, Entity)> buildScene = [&](uint32_t index, Entity parentEntity)
		{
			const auto& nodeDesc = desc.nodes[index];
			Entity entity = CreateEntity(nodeDesc.name);
			auto& transform = m_Registry.get<TransformComponent>(entity);
			transform = TransformComponent(nodeDesc.translation, nodeDesc.scale, nodeDesc.rotation);

			if (nodeDesc.meshIndex >= 0 && nodeDesc.meshIndex < desc.submeshes.size())
			{
				const auto& meshDesc = desc.submeshes[nodeDesc.meshIndex];
				auto mesh = resources.Meshes[nodeDesc.meshIndex];
				entity.AddComponent<MeshComponent>(mesh);
				if (meshDesc.materialIndex >= 0 && meshDesc.materialIndex < desc.materials.size())
				{
					auto material = resources.Materials[meshDesc.materialIndex];
					entity.AddComponent<MaterialComponent>(material);
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
	void Scene::CollectRenderObjectsRecursive(const Node* node, std::vector<RenderObject>& renderList)
	{
		//if (node->GetMesh() && node->GetMaterial())
		//{
		//	auto renderObject = RenderObject::Create(node->GetMesh(), node->GetMaterial());
		//	/*renderObject.mesh = node->GetMesh();
		//	renderObject.material = node->GetMaterial();
		//	renderObject.transform = node->GetWorldTransform();*/
		//	renderObject->transformComponent.SetTransform(node->GetWorldTransform());
		//	renderList.push_back(*renderObject);
		//}

		//for (const auto& child : node->GetChildren())
		//{
		//	CollectRenderObjectsRecursive(child.get(), renderList);
		//}
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