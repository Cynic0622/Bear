#include "bearpch.h"

#include "Instance.h"
#include "Bear/Log.h"
#include "Validation.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

namespace Bear {

	Instance::Instance(const std::string& appName, const std::string& engineName, bool enableValidation)
	{
		m_AppName = appName;
		m_EngineName = engineName;
		m_EnableValidation = enableValidation;
		CreateInstance();

		if (m_EnableValidation) {
			m_Validation = std::make_unique<Validation>(m_Instance);
		}
	}
	Instance::~Instance()
	{
		if (m_Validation) {
			m_Validation.reset();
#ifdef BEAR_DEBUG
			BEAR_CORE_INFO("Vulkan validation destroyed successfully.");
#endif // BEAR_DEBUG
		}
		if (m_Instance) {
			vkDestroyInstance(m_Instance, nullptr);
			m_Instance = VK_NULL_HANDLE;
#ifdef BEAR_DEBUG
			BEAR_CORE_INFO("Vulkan instance destroyed successfully.");
#endif // BEAR_DEBUG
		}
	}
	void Instance::CreateInstance()
	{
		if (m_EnableValidation && !Validation::CheckSupport()) {
			BEAR_CORE_ERROR("Validation layers requested, but not available!");
		}

		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = m_AppName.c_str();
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = m_EngineName.c_str();
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		// 设置扩展
		auto extensions = GetRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		// 设置验证层
		if (m_EnableValidation) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(Validation::Layers.size());
			createInfo.ppEnabledLayerNames = Validation::Layers.data();
		} else {
			createInfo.enabledLayerCount = 0;
		}
		// 创建实例
		BEAR_CORE_ASSERT(vkCreateInstance(&createInfo, nullptr, &m_Instance) == VK_SUCCESS, "Failed to create Vulkan instance.");

#ifdef BEAR_DEBUG
		BEAR_CORE_INFO("Vulkan instance created successfully with app name: {0}, engine name: {1}", m_AppName, m_EngineName);
#endif // BEAR_DEBUG
	}
	
	std::vector<const char*> Instance::GetRequiredExtensions() const
	{
		uint32_t extensionCount = 0;
		BEAR_CORE_ASSERT(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr) == VK_SUCCESS, "Failed to enumerate Vulkan instance extensions.");
		std::vector<VkExtensionProperties> extensions(extensionCount);
		
		BEAR_CORE_ASSERT(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data()) == VK_SUCCESS, "Failed to enumerate Vulkan instance extensions.");

		std::vector<const char*> requiredExtensions;
		/*for (const auto& extension : extensions) {
			requiredExtensions.push_back(extension.extensionName);
		}*/

		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		BEAR_CORE_ASSERT(glfwExtensions != nullptr, "Failed to get GLFW required instance extensions.");
		for (uint32_t i = 0; i < glfwExtensionCount; ++i) {
			requiredExtensions.push_back(glfwExtensions[i]);
		}
		if (m_EnableValidation) {
			requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		BEAR_CORE_ASSERT(!requiredExtensions.empty(), "No Vulkan instance extensions found.");
		if (requiredExtensions.size() > 0) {
#ifdef BEAR_DEBUG
			BEAR_CORE_INFO("Vulkan instance extensions found: {0}", requiredExtensions.size());
#endif // BEAR_DEBUG
			return requiredExtensions;
		}
		else {
			BEAR_CORE_ERROR("No Vulkan instance extensions found.");
		}
		return std::vector<const char*>();
	}
}