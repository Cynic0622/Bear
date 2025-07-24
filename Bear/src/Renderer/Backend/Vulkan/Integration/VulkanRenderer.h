#pragma once

#include <vulkan/vulkan.h>

struct GLFWwindow;

namespace Bear {

	class VulkanInstance;
	class VulkanSurface;
	class VulkanDevice;
	class VulkanSwapchain;
	class VulkanRenderPass;
	class VulkanPipeline;
	class VulkanCommandPool;
	class VulkanCommandBuffer;
	class VulkanSemaphore;
	class VulkanFence;


	class VulkanRenderer {

	public:
		VulkanRenderer(GLFWwindow* window);
		~VulkanRenderer();

		VulkanRenderer(const VulkanRenderer&) = delete;
		VulkanRenderer& operator=(const VulkanRenderer&) = delete;

		void DrawFrame();
		void OnWindowResized();

	private:

		void Initialize();
		void Cleanup();
		void RecreateSwapchain();
		void RecordCommandBuffer(uint32_t imageIndex);

	private:
		std::unique_ptr<VulkanInstance> m_Instance;
		GLFWwindow* m_Window;
		std::unique_ptr<VulkanSurface> m_Surface;
		std::unique_ptr<VulkanDevice> m_Device;
		std::unique_ptr<VulkanSwapchain> m_Swapchain;
		std::unique_ptr<VulkanRenderPass> m_RenderPass;
		std::unique_ptr<VulkanPipeline> m_Pipeline;

		VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;

		std::unique_ptr<VulkanCommandPool> m_CommandPool;
		std::vector<std::unique_ptr<VulkanCommandBuffer>> m_CommandBuffers;

		const int MAX_FRAMES_IN_FLIGHT = 2;
		uint32_t m_CurrentFrame = 0;

		std::vector<std::unique_ptr<VulkanSemaphore>> m_ImageAvailableSemaphores;
		std::vector<std::unique_ptr<VulkanSemaphore>> m_RenderFinishedSemaphores;
		std::vector<std::unique_ptr<VulkanFence>> m_InFlightFences;

		bool m_FramebufferResized = false;
	};
}