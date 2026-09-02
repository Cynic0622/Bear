#pragma once

#include "Bear/Layer.h"
#include "Bear/Events/Event.h"
#include "Bear/Events/ApplicationEvent.h"
#include "Bear/Events/KeyEvent.h"
#include "Bear/Events/MouseEvent.h"

#include <vulkan/vulkan.h>
#include <functional>

namespace Bear
{
	class RHICommandList;
	struct RenderStats;
	struct RenderStates;
}

// Forward declarations
struct GLFWwindow;

namespace Bear {
	class RHIDevice;

	// Forward declarations
    class Device;
    
    class BEAR_API GuiLayer : public Layer {
    public:
        GuiLayer();
    	~GuiLayer() override = default;

        GuiLayer(const GuiLayer&) = delete;
		GuiLayer& operator=(const GuiLayer&) = delete;

    	void OnAttach() override;
    	void OnDetach() override;
    	void OnUpdate(float deltaTime) override;
    	void OnEvent(Event& event) override;
        void OnRender() const override;

        void SetOnModelSwitchCallback(std::function<void(const std::string&)> callback);
        void SetOnReloadModelCallback(std::function<void()> callback);
        void SetOnInstanceCountChangeCallback(std::function<void(uint32_t)> callback);
        void SetDrawStatsProvider(std::function<RenderStats()> provider);
        void SetRenderStatesProvider(std::function<RenderStates()> provider);

    private:

		bool OnMouseButtonPress(MouseButtonPressedEvent& event);
		bool OnWindowResize(WindowResizeEvent& event);


    private:
		RHIDevice* m_Device = nullptr;
        
        // ImGui专用的Vulkan资源
        VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;

        static constexpr size_t kFrameHistorySize = 120;
        std::vector<float> m_FrameTimeHistory;
        size_t m_HistoryIndex = 0;
        float m_AccumulatedFrameTime = 0.0f;
        float m_MinFrameTime = std::numeric_limits<float>::max();
        float m_MaxFrameTime = 0.0f;
        float m_MovingAvgFrameTime = 0.0f;

        std::function<void(const std::string&)> m_OnModelSwitch;
        std::function<void()> m_OnReloadModel;
        std::function<void(uint32_t)> m_OnInstanceCountChange;
        std::function<RenderStats()> m_DrawStatsProvider;
        std::function<RenderStates()> m_RenderStatesProvider;
    };
}