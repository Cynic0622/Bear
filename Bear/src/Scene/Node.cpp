#include "bearpch.h"
#include "Node.h"

namespace Bear
{
	OitNode::OitNode(std::string name)
		:m_Name(std::move(name))
	{
	}
	void OitNode::SetPosition(const glm::vec3& position)
	{
		m_Position = position;
		SetDirty();
	}
	void OitNode::SetRotation(const glm::quat& rotation)
	{
		m_Rotation = rotation;
		SetDirty();
	}
	void OitNode::SetScale(const glm::vec3& scale)
	{
		m_Scale = scale;
		SetDirty();
	}
	void OitNode::AddChild(std::unique_ptr<OitNode> child)
	{
		child->m_Parent = this;
		m_Children.push_back(std::move(child));
	}
	void OitNode::Update(bool forceUpdate)
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
	void OitNode::UpdateWorldTransform()
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
	void OitNode::SetDirty()
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
