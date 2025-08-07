#include "bearpch.h"
#include "Node.h"
#include "Scene.h"
#include "RenderObject.h"

namespace Bear
{
	Scene::Scene()
	{
		m_RootNode = std::make_unique<Node>("rootNode");
	}
	void Scene::Update() const
	{
		m_RootNode->Update();
	}
	void Scene::CollectRenderObjects(std::vector<RenderObject>& ObjectsList)
	{
		if (m_RootNode)
		{
			CollectRenderObjectsRecursive(m_RootNode.get(), ObjectsList);
		}
		else
		{
			BEAR_CORE_ERROR("Scene has no root node!")
		}
	}
	void Scene::CollectRenderObjectsRecursive(const Node* node, std::vector<RenderObject>& renderList)
	{
		if (node->GetMesh() && node->GetMaterial())
		{
			auto renderObject = RenderObject::Create(node->GetMesh(), node->GetMaterial());
			/*renderObject.mesh = node->GetMesh();
			renderObject.material = node->GetMaterial();
			renderObject.transform = node->GetWorldTransform();*/
			renderObject->transformComponent.SetTransform(node->GetWorldTransform());
			renderList.push_back(*renderObject);
		}

		for (const auto& child : node->GetChildren())
		{
			CollectRenderObjectsRecursive(child.get(), renderList);
		}
	}
}