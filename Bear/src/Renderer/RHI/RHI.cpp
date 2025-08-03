#include "bearpch.h"
#include "RHI.h"
#include "RHIDevice.h"
#include "Core/VulkanInstance.h"
#include "Presentation/VulkanSurface.h"
namespace Bear {
	std::unique_ptr<RHIDevice> Bear::CreateDevice(GraphicsAPI api, const RHIPlatformData& platformData)
	{

		switch (api)
		{
			case GraphicsAPI::Vulkan:
				auto instance = std::make_unique<VulkanInstance>("app", "Bear", true);
				auto surface = std::make_unique<VulkanSurface>(*instance, static_cast<GLFWwindow*>(platformData.windowHandle));
				return std::make_unique<RHIDevice>(std::move(instance), std::move(surface));
		}
	}
}
