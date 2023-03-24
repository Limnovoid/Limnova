#pragma once

#include "Core/Layer.h"

#include "Events/ApplicationEvent.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"


namespace Limnova
{

    class LIMNOVA_API ImGuiLayer : public Layer
    {
    public:
        /*** NOTE : enums must correspond to AddFontFromFileTTF() calls in ImGuiLayer::OnAttach() ***/
        enum FontIndex : uint32_t
        {
            Regular     = 0,
            Bold        = 1
        };
    public:
        ImGuiLayer();
        ~ImGuiLayer();

        void OnAttach() override;
        void OnDetach() override;
        void OnImGuiRender() override;
        void OnEvent(Event& e) override;

        void Begin();
        void End();

        void SetBlockEvents(bool block) { m_BlockEvents = block; }

        void SetDarkTheme();
    private:
        bool m_BlockEvents = true;
        float m_Time = 0.f;
    };

}
