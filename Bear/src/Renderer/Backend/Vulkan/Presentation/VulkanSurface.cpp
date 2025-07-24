#include "bearpch.h"

#include "VulkanSurface.h"
#include "Core/VulkanInstance.h"

namespace Bear {

	VulkanSurface::VulkanSurface(const VulkanInstance& instance, GLFWwindow* window)
		:m_Instance(instance), m_Window(window)
	{
		BEAR_CORE_ASSERT(window, "Window cannot be null when creating a Vulkan surface.");
		BEAR_CORE_ASSERT(glfwCreateWindowSurface(m_Instance.GetHandle(), window, nullptr, &m_Surface) == VK_SUCCESS, "Failed to create Vulkan surface.");
#ifdef BEAR_DEBUG
		BEAR_CORE_INFO("Vulkan surface created successfully.");
#endif // BEAR_DEBUG
	}

	VulkanSurface::~VulkanSurface() {
		if (m_Surface != VK_NULL_HANDLE) {
			vkDestroySurfaceKHR(m_Instance.GetHandle(), m_Surface, nullptr);
			m_Surface = VK_NULL_HANDLE;
#ifdef BEAR_DEBUG
			BEAR_CORE_INFO("Vulkan surface destroyed successfully.");
#endif // BEAR_DEBUG
		}
	}
}