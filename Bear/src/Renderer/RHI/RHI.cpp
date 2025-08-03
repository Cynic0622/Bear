#include "bearpch.h"
#include "RHI.h"
#include "RHIDevice.h"
#include "Core/VulkanDevice.h"

namespace Bear {
	std::unique_ptr<RHIDevice> Bear::CreateDevice(GraphicsAPI api, const RHIPlatformData& platformData)
	{

		switch (api)
		{
		case GraphicsAPI::Vulkan: {
			return std::make_unique<VulkanDevice>(static_cast<GLFWwindow*>(platformData.windowHandle));
		}
		}
	}
}
