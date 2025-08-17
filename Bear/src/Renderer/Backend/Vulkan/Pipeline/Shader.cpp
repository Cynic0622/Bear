#include "bearpch.h"

#include "Shader.h"
#include "Core/Device.h"

namespace Bear {
	Shader::Shader(const Device& device, const std::string& filepath, VkShaderStageFlagBits stage)
		: m_Device(device), m_Stage(stage)
	{
		auto shaderCode = ReadFile(filepath);
		CreateShaderModule(shaderCode);
#ifdef BEAR_DEBUG
		BEAR_CORE_INFO("Vulkan Shader loaded and module created:{}", filepath);
#endif // BEAR_DEBUG

	}

	Shader::~Shader()
	{
		if (m_ShaderModule != VK_NULL_HANDLE) {
			vkDestroyShaderModule(m_Device.GetDevice(), m_ShaderModule, nullptr);
			m_ShaderModule = VK_NULL_HANDLE;
#ifdef BEAR_DEBUG
			BEAR_CORE_INFO("Vulkan Shader module destroyed successfully.");
#endif // BEAR_DEBUG
		}
	}

	VkPipelineShaderStageCreateInfo Shader::GetStageCreateInfo() const
	{
		BEAR_CORE_ASSERT(m_ShaderModule != VK_NULL_HANDLE, "Shader module is not created yet!");

		VkPipelineShaderStageCreateInfo stageCreateInfo = {};
		stageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stageCreateInfo.stage = m_Stage;
		stageCreateInfo.module = m_ShaderModule;
		stageCreateInfo.pName = "main"; // Entry point name, usually "main" for shaders
		stageCreateInfo.pSpecializationInfo = nullptr; // Optional specialization info, can be set to nullptr if not used

		return stageCreateInfo;
	}

	std::vector<char> Shader::ReadFile(const std::string& filepath)
	{
		std::ifstream file(filepath, std::ios::ate | std::ios::binary);
		BEAR_CORE_ASSERT(file.is_open(), "Failed to open file: {}", filepath);

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();

		return buffer;
	}

	void Shader::CreateShaderModule(const std::vector<char>& code)
	{
		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
		
		BEAR_CORE_ASSERT(vkCreateShaderModule(m_Device.GetDevice(), &createInfo, nullptr, &m_ShaderModule) == VK_SUCCESS,
			"failed to create shader module!");
	}
}