#pragma once

namespace Bear
{
    struct RenderContext;
    class RHICommandList;
	class IRenderPass
    {
        public:
        virtual ~IRenderPass() = default;

        virtual void Setup(RenderContext* context) = 0;
        virtual void Execute(RHICommandList* cmd) = 0;
        virtual void Resize() = 0;
		virtual void Cleanup() = 0;
    };
}