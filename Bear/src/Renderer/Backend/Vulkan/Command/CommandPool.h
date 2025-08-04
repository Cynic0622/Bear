#pragma once

#include <vulkan/vulkan.h>

namespace Bear {

	class Device;
	class CommandPool
	{
	public:
		CommandPool(const Device& device);
		~CommandPool();

		CommandPool(const CommandPool&) = delete;
		CommandPool& operator=(const CommandPool&) = delete;


		inline VkCommandPool GetHandle() const { return m_CommandPool; }
		inline const Device& GetDevice() const { return m_Device; }

	private:
		const Device& m_Device;
		VkCommandPool m_CommandPool = VK_NULL_HANDLE;
	};
}