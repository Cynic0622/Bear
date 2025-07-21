#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <memory>
#include <vector>


namespace Bear {

	class VulkanValidation;

	class VulkanInstance {

	public:
		VulkanInstance(const std::string& appName, const std::string& engineName, bool enableValidation);
		~VulkanInstance();

		// 禁止拷贝和移动，保证实例的唯一性
		VulkanInstance(const VulkanInstance&) = delete;
		VulkanInstance& operator=(const VulkanInstance&) = delete;
		VulkanInstance(VulkanInstance&&) = delete;
		VulkanInstance& operator=(VulkanInstance&&) = delete;

		inline VkInstance GetHandle() const { return m_Instance; }
		inline bool IsValidationEnabled() const { return m_EnableValidation; }

	private:

		VkInstance m_Instance = VK_NULL_HANDLE;
		std::unique_ptr<VulkanValidation> m_Validation;
		bool m_EnableValidation = false;

		

		std::string m_AppName;
		std::string m_EngineName;


	private:
		void CreateInstance();

		std::vector<const char*> GetRequiredExtensions() const;

	};
}