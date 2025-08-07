#include "bearpch.h"
#include "Node.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Bear
{
	Node::Node(std::string name)
		:m_Name(std::move(name))
	{
	}
	void Node::SetPosition(const glm::vec3& position)
	{
		m_Position = position;
		SetDirty();
	}
	void Node::SetRotation(const glm::quat& rotation)
	{
		m_Rotation = rotation;
		SetDirty();
	}
	void Node::SetScale(const glm::vec3& scale)
	{
		m_Scale = scale;
		SetDirty();
	}
	void Node::AddChild(std::unique_ptr<Node> child)
	{
		child->m_Parent = this;
		m_Children.push_back(std::move(child));
	}
	void Node::Update(bool forceUpdate)
	{
		if (forceUpdate || m_IsDirty)
		{
			UpdateWorldTransform();
			m_IsDirty = false;
		}
		// update children
		for (auto& child : m_Children)
		{
			child->Update(forceUpdate);
		}
	}
	void Node::UpdateWorldTransform()
	{
		glm::mat4 localTransform = glm::translate(glm::mat4(1.0f), m_Position) *
			glm::mat4_cast(m_Rotation) *
			glm::scale(glm::mat4(1.0f), m_Scale);
		if (m_Parent)
		{
			m_WorldTransform = m_Parent->GetWorldTransform() * localTransform;
		}
		else
		{
			m_WorldTransform = localTransform;
		}
	}
	void Node::SetDirty()
	{
		if (!m_IsDirty)
		{
			m_IsDirty = true;
			for (auto& child : m_Children)
			{
				child->SetDirty();
			}
		}
	}
}
