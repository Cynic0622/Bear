#pragma once
#include <vulkan/vulkan.h>

namespace Bear {

	class Device;

	class Fence {

	public:
		Fence(const Device& device, bool signaled = false);
		~Fence();

		Fence(const Fence&) = delete;
		Fence& operator=(const Fence&) = delete;

		void Wait(uint64_t timeout = UINT64_MAX);
		void Reset();

		inline VkFence GetHandle() const { return m_Fence; }

	private:
		const Device& m_Device;
		VkFence m_Fence = VK_NULL_HANDLE;
	};
}