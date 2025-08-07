#pragma once

namespace Bear
{
	class Node;
	class RenderObject;

	class Scene
	{
	public:
		Scene();
		~Scene() = default;

		// update the all the transforms in the scene
		void Update() const;

		void CollectRenderObjects(std::vector<RenderObject>& ObjectsList);

		Node* GetRootNode() const { return m_RootNode.get(); }

	private:
		std::unique_ptr<Node> m_RootNode; // te root node of the scene graph

	private:
		void CollectRenderObjectsRecursive(const Node* node, std::vector<RenderObject>& renderList);
	};
}
