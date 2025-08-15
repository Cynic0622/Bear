#include "bearpch.h"
#include "Node.h"
#include "Scene.h"

#include "Application.h"
#include "AssetLoader.h"
#include "Component.h"
#include "RenderObject.h"
#include "Entity.h"
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
			auto& hierarchy = view.get<HierarchyComponent>(entity);
			if (hierarchy.Parent)
			{
				auto& parentTransform = hierarchy.Parent.GetComponent<TransformComponent>();
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