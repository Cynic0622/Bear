#pragma once

namespace Bear {

	class RHIRenderPass
	{
	public:
		virtual ~RHIRenderPass() = default;

		// get origin handle
		virtual void* GetNativeHandle() const = 0;
	};
}