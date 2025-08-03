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
	class VulkanBuffer;
	class Mesh;
	class Material;
	class RenderObject;
	struct UniformBufferObject;

	class VulkanRenderer {

	public:
		VulkanRenderer(GLFWwindow* window);
		~VulkanRenderer();

		VulkanRenderer(const VulkanRenderer&) = delete;
		VulkanRenderer& operator=(const VulkanRenderer&) = delete;

		void DrawFrame();
		void OnWindowResized();

		inline VulkanInstance* GetVulkanInstance() const { return m_Instance.get(); }
		inline VulkanDevice* GetVulkanDevice() const { return m_Device.get(); }
		inline VulkanSurface* GetVulkanSurface() const { return m_Surface.get(); }
		inline VulkanSwapchain* GetVulkanSwapchain() const { return m_Swapchain.get(); }
		inline VulkanRenderPass* GetVulkanRenderPass() const { return m_RenderPass.get(); }
		inline VulkanPipeline* GetVulkanPipeline() const { return m_Pipeline.get(); }
		inline VulkanCommandPool* GetVulkanCommandPool() const { return m_CommandPool.get(); }
		inline const std::vector<std::unique_ptr<VulkanCommandBuffer>>& GetCommandBuffers() const { return m_CommandBuffers; }
		inline std::vector< VulkanSemaphore*> GetImageAvailableSemaphores() const { 
			std::vector<VulkanSemaphore*> m_ImageAvailableSemaphoresHandles;
			for (const auto& semaphore : m_ImageAvailableSemaphores) {
				m_ImageAvailableSemaphoresHandles.push_back(semaphore.get());
			}
			return m_ImageAvailableSemaphoresHandles; 
		}

	private:

		void Initialize();
		void Cleanup();
		void RecreateSwapchain();
		void RecordCommandBuffer(uint32_t imageIndex, UniformBufferObject& ubo);

	private:
		std::unique_ptr<VulkanInstance> m_Instance;
		GLFWwindow* m_Window;
		std::unique_ptr<VulkanSurface> m_Surface;
		//std::unique_ptr<Mesh> m_Mesh; // temp, should be removed later
		// std::unique_ptr<VulkanBuffer> m_VertexBuffer; // temp
		// std::unique_ptr<VulkanBuffer> m_IndexBuffer; // temp

		std::shared_ptr<Mesh> m_SquareMesh;
		std::shared_ptr<Material> m_SimpleMaterial;
		std::vector<std::unique_ptr<RenderObject>> m_RenderObjects;

		std::unique_ptr<VulkanSwapchain> m_Swapchain;
		std::unique_ptr<VulkanRenderPass> m_RenderPass;
		std::unique_ptr<VulkanPipeline> m_Pipeline;

		VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;

		

		bool m_FramebufferResized = false;
		std::unique_ptr<VulkanDevice> m_Device;
	};
}