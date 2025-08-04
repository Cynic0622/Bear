#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <memory>
#include <vector>


namespace Bear {

	class Validation;

	class Instance {

	public:
		Instance(const std::string& appName, const std::string& engineName, bool enableValidation);
		~Instance();

		// 禁止拷贝和移动，保证实例的唯一性
		Instance(const Instance&) = delete;
		Instance& operator=(const Instance&) = delete;
		Instance(Instance&&) = delete;
		Instance& operator=(Instance&&) = delete;

		inline VkInstance GetHandle() const { return m_Instance; }
		inline bool IsValidationEnabled() const { return m_EnableValidation; }

	private:

		VkInstance m_Instance = VK_NULL_HANDLE;
		std::unique_ptr<Validation> m_Validation;
		bool m_EnableValidation = false;

		

		std::string m_AppName;
		std::string m_EngineName;


	private:
		void CreateInstance();

		std::vector<const char*> GetRequiredExtensions() const;

	};
}