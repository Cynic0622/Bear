#pragma once
#include "RHITypes.h"

namespace Bear {
	class RHIDevice;
	std::unique_ptr<RHIDevice> CreateDevice(GraphicsAPI api, const RHIPlatformData& platformData);
}