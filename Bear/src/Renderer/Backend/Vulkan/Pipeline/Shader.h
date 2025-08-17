#pragma once

#include <vulkan/vulkan.h>
#include <string>
namespace Bear {
	
	class Device;

	class Shader
	{
	public:
		Shader(const Device& device, const std::string& filepath, VkShaderStageFlagBits stage);
		~Shader();

		// 禁止拷贝和移动
		Shader(const Shader&) = delete;
		Shader& operator=(const Shader&) = delete;
		Shader(Shader&&) = delete;
		Shader& operator=(Shader&&) = delete;

		// 获取着色器模块和阶段信息，用于管线创建
		VkShaderModule GetModule() const { return m_ShaderModule; }
		VkShaderStageFlagBits GetStage() const { return m_Stage; }

		VkPipelineShaderStageCreateInfo GetStageCreateInfo() const;

	private:
		const Device& m_Device;
		VkShaderModule m_ShaderModule = VK_NULL_HANDLE;
		VkShaderStageFlagBits m_Stage;

	private:
		static std::vector<char> ReadFile(const std::string& filepath);
		void CreateShaderModule(const std::vector<char>& code);
	};
}