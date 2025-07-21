#pragma once

#include <vulkan/vulkan.h>
#include <string>
namespace Bear {
	
	class VulkanDevice;

	class VulkanShader
	{
	public:
		VulkanShader(const VulkanDevice& device, const std::string& filepath, VkShaderStageFlagBits stage);
		~VulkanShader();

		// 禁止拷贝和移动
		VulkanShader(const VulkanShader&) = delete;
		VulkanShader& operator=(const VulkanShader&) = delete;
		VulkanShader(VulkanShader&&) = delete;
		VulkanShader& operator=(VulkanShader&&) = delete;

		// 获取着色器模块和阶段信息，用于管线创建
		VkShaderModule GetModule() const { return m_ShaderModule; }
		VkShaderStageFlagBits GetStage() const { return m_Stage; }

		VkPipelineShaderStageCreateInfo GetStageCreateInfo() const;

	private:
		const VulkanDevice& m_Device;
		VkShaderModule m_ShaderModule = VK_NULL_HANDLE;
		VkShaderStageFlagBits m_Stage;

	private:
		static std::vector<char> ReadFile(const std::string& filepath);
		void CreateShaderModule(const std::vector<char>& code);
	};
}