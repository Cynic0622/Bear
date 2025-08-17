#pragma once
#include <vulkan/vulkan.h>
#include <vector>

namespace Bear {

	struct PipelineConfigInfo {
		// 这是创建管线所必须的
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		VkRenderPass renderPass = VK_NULL_HANDLE;

		// 可配置的状态
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
		VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
		VkPipelineMultisampleStateCreateInfo multisampleInfo{};
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		VkPipelineColorBlendStateCreateInfo colorBlendInfo{};
		VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
		VkPipelineViewportStateCreateInfo viewportInfo{}; // 视口和裁剪矩形

		// 用于动态状态，允许在录制命令时更改某些管线状态
		std::vector<VkDynamicState> dynamicStateEnables;
		VkPipelineDynamicStateCreateInfo dynamicStateInfo{};

		// 禁止拷贝，因为这个结构体可能持有非所有权的句柄
		PipelineConfigInfo(const PipelineConfigInfo&) = delete;
		PipelineConfigInfo& operator=(const PipelineConfigInfo&) = delete;

		// 提供一个默认配置的静态函数
		static void GetDefaultConfig(PipelineConfigInfo& configInfo);
	};
	
}