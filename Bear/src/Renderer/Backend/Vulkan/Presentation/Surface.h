#pragma once

#include <vulkan/vulkan.h>

// 前向声明GLFWwindow，避免在头文件中包含GLFW的头文件
struct GLFWwindow;

namespace Bear {

	class Instance; // 前向声明，表示依赖关系

	class Surface {
	public:
		// 构造函数需要一个Vulkan实例和一个窗口指针来创建表面
		Surface(const Instance& instance, GLFWwindow* window);
		~Surface();

		// 禁止拷贝和移动，因为 VkSurfaceKHR 是一个与特定窗口绑定的独特资源
		Surface(const Surface&) = delete;
		Surface& operator=(const Surface&) = delete;
		Surface(Surface&&) = delete;
		Surface& operator=(Surface&&) = delete;

		// 公共接口，用于获取底层的 VkSurfaceKHR 句柄
		inline VkSurfaceKHR GetHandle() const { return m_Surface; }
		inline void* GetNativeWindow() const { return m_Window; }

	private:
		// 保存一份实例的引用，用于在析构时销毁表面
		const Instance& m_Instance;
		void* m_Window = nullptr; // 窗口指针，用于创建表面
		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
	};

} // namespace Bear