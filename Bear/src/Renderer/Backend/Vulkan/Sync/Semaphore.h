#pragma once
#include <vulkan/vulkan.h>

namespace Bear {

	class Device;
	class Semaphore {
		
	public:
		Semaphore(const Device& device, const void* pNext = nullptr, const VkSemaphoreCreateFlags& flags = 0);
		~Semaphore();

		Semaphore(const Semaphore&) = delete;
		Semaphore& operator=(const Semaphore&) = delete;

		inline VkSemaphore GetHandle() const { return m_Semaphore; }

	private:
		const Device& m_Device;
		VkSemaphore m_Semaphore = VK_NULL_HANDLE;
	};
}