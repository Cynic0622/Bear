#include "bearpch.h"
#include "GuiLayer.h"
#include "Bear/Application.h"
#include "Scene/SceneLayer.h"

namespace Bear {
    
    GuiLayer::GuiLayer() : Layer("GuiLayer") {
        m_FrameTimeHistory.resize(kFrameHistorySize, 0.0f);
    }

    
    void GuiLayer::OnAttach() {

    }
    
    void GuiLayer::OnDetach() {
    }

    bool GuiLayer::OnMouseButtonPress(MouseButtonPressedEvent& event)
    {
        ImGuiIO& io = ImGui::GetIO();
		return io.WantCaptureMouse; // if ImGui wants to capture the mouse input, return true
    }

    bool GuiLayer::OnWindowResize(WindowResizeEvent& event)
    {
        return false;
    }

    void GuiLayer::SetOnModelSwitchCallback(std::function<void(const std::string&)> callback)
    {
        m_OnModelSwitch = std::move(callback);
    }

    void GuiLayer::OnUpdate(float deltaTime) {
        float dtMs = deltaTime * 1000.0f;

        m_AccumulatedFrameTime -= m_FrameTimeHistory[m_HistoryIndex];
        m_FrameTimeHistory[m_HistoryIndex] = dtMs;
        m_AccumulatedFrameTime += dtMs;
        m_HistoryIndex = (m_HistoryIndex + 1) % kFrameHistorySize;
        m_MovingAvgFrameTime = m_AccumulatedFrameTime / (float)kFrameHistorySize;

        if (dtMs < m_MinFrameTime) m_MinFrameTime = dtMs;
        if (dtMs > m_MaxFrameTime) m_MaxFrameTime = dtMs;

        // --- Model Switcher ---
        if (m_OnModelSwitch)
        {
            ImGui::Begin("Models");
            static int current = 0;
            const auto& models = SceneLayer::GetModelList();
            std::vector<const char*> names;
            for (const auto& m : models)
                names.push_back(m.first);
            ImGui::Combo("Model", &current, names.data(), (int)names.size());
            if (ImGui::Button("Load"))
            {
                m_OnModelSwitch(models[current].second);
            }
            ImGui::End();
        }

        ImGui::Begin("Renderer Info");
        ImGui::Text("Graphics api: Vulkan");
        ImGui::SeparatorText("Frame Timing");
        ImGui::Text("Frame time: %.3f ms", dtMs);
        ImGui::Text("FPS: %.1f", 1.0f / deltaTime);
        ImGui::Text("Avg (120 frames): %.3f ms  (%.1f FPS)", m_MovingAvgFrameTime, 1000.0f / m_MovingAvgFrameTime);
        ImGui::Text("Min: %.3f ms   Max: %.3f ms", m_MinFrameTime, m_MaxFrameTime);

        static constexpr float kLabelWidth = 48.0f;
        static constexpr float kGraphHeight = 100.0f;

        std::array<float, kFrameHistorySize> plotData;
        for (size_t i = 0; i < kFrameHistorySize; ++i)
        {
            size_t idx = (m_HistoryIndex + i) % kFrameHistorySize;
            plotData[i] = m_FrameTimeHistory[idx];
        }

        float graphWidth = ImGui::GetContentRegionAvail().x - kLabelWidth;
        ImVec2 graphOrigin = ImGui::GetCursorScreenPos();
        graphOrigin.x += kLabelWidth;
        ImVec2 graphEnd(graphOrigin.x + graphWidth, graphOrigin.y + kGraphHeight);

        float yMin = 0.0f;
        auto toFps = [](float ms) { return ms > 0.0f ? 1000.0f / ms : 999.0f; };
        float yMax = (std::max)(240.0f, toFps(*std::min_element(plotData.begin(), plotData.end())) * 1.1f);
        ImDrawList* draw = ImGui::GetWindowDrawList();

        draw->AddRectFilled(graphOrigin, graphEnd, IM_COL32(12, 12, 16, 200));
        draw->AddRect(graphOrigin, graphEnd, IM_COL32(60, 60, 80, 120), 0.0f, 0, 1.0f);

        static constexpr int kGridLines = 4;
        for (int i = 0; i <= kGridLines; ++i)
        {
            float t = (float)i / (float)kGridLines;
            float ly = graphOrigin.y + kGraphHeight * (1.0f - t);
            draw->AddLine(ImVec2(graphOrigin.x, ly), ImVec2(graphEnd.x, ly),
                IM_COL32(50, 50, 65, 100), 0.5f);
            char label[16];
            snprintf(label, sizeof(label), "%.0f", yMin + (yMax - yMin) * t);
            draw->AddText(ImVec2(graphOrigin.x - kLabelWidth + 4.0f, ly - 7.0f),
                IM_COL32(160, 160, 170, 220), label);
        }

        float stepX = graphWidth / (float)(kFrameHistorySize - 1);
        std::vector<ImVec2> linePts;
        linePts.reserve(kFrameHistorySize);
        for (size_t i = 0; i < kFrameHistorySize; ++i)
        {
            float fps = toFps(plotData[i]);
            float t = (fps - yMin) / (yMax - yMin);
            linePts.emplace_back(graphOrigin.x + (float)i * stepX,
                graphOrigin.y + kGraphHeight * (1.0f - t));
        }

        std::vector<ImVec2> fillPts = linePts;
        fillPts.emplace_back(graphEnd.x, graphEnd.y);
        fillPts.emplace_back(graphOrigin.x, graphEnd.y);
        draw->AddConvexPolyFilled(fillPts.data(), (int)fillPts.size(),
            IM_COL32(70, 160, 240, 55));
        draw->AddPolyline(linePts.data(), (int)linePts.size(),
            IM_COL32(70, 160, 240, 255), false, 2.0f);

        float curFps = toFps(plotData.back());
        char ov[32];
        snprintf(ov, sizeof(ov), "%.0f FPS", curFps);
        ImVec2 ovPos(graphEnd.x + 6.0f, linePts.back().y - 7.0f);
        draw->AddText(ovPos, IM_COL32(70, 160, 240, 255), ov);

        ImGui::Dummy(ImVec2(kLabelWidth + graphWidth, kGraphHeight));

        ImGui::End();
        
    }
    
    void GuiLayer::OnEvent(Event& event) {

		EventDispatcher dispatcher(event);
        dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent& e)
            {
				return this->OnMouseButtonPress(e);
            });
    }

    void GuiLayer::OnRender() const
    {
    }

}