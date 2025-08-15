#pragma once

#include "Scene.h"
namespace Bear
{

	class Entity
	{
	public:
		//Entity() = default;
		Entity(entt::entity handle, Scene* scene) : m_EntityHandle(handle), m_Scene(scene) {}

		// add component
		template<typename T, typename... TArgs> // T is the component type, TArgs are the constructor arguments for the component.
		T& AddComponent(TArgs&&... args) {
		BEAR_CORE_ASSERT(!HasComponent<T>(), "Entity already has component!");
			// std::forward<TArgs>(args)... allows perfect forwarding of arguments, reduces unnecessary copies.
			return m_Scene->m_Registry.emplace<T>(m_EntityHandle, std::forward<TArgs>(args)...);
		}
		// get component
		template<typename T>
		T& GetComponent() {
			BEAR_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
			return m_Scene->m_Registry.get<T>(m_EntityHandle);
		}
		// check if entity has component
		template<typename T>
		bool HasComponent() {
			return m_Scene->m_Registry.all_of<T>(m_EntityHandle);
		}
		// remove component
		template<typename T>
		void RemoveComponent() {
			BEAR_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
			m_Scene->m_Registry.remove<T>(m_EntityHandle);
		}
		// this overload is to allow the use of Entity as a boolean in conditions.
		operator bool() const { return m_EntityHandle != entt::null; }
		operator entt::entity() const { return m_EntityHandle; }
		bool operator!=(entt::entity other) const {
			return m_EntityHandle != other;
		}
	private:
		entt::entity m_EntityHandle{ entt::null };
		Scene* m_Scene{ nullptr };
	};
}
