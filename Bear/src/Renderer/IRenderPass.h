#pragma once
#include "RenderObject.h"
namespace Bear
{
    struct RenderContext;
    class RHICommandList;
	struct RenderObject;
	class IRenderPass
    {
        public:
        virtual ~IRenderPass() = default;

        virtual void Setup(RenderContext* context) = 0;
        virtual void Execute(RHICommandList* cmd, std::vector<RenderObject> renderObjects = {}) = 0;
        virtual void Resize() = 0;
		virtual void Cleanup() = 0;
    };
}