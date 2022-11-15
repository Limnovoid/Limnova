#pragma once

#include "Layer.h"

#include "Events/ApplicationEvent.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"


namespace Limnova
{

    class LIMNOVA_API ImGuiLayer : public Layer
    {
    public:
        ImGuiLayer();
        ~ImGuiLayer();

        void OnAttach() override;
        void OnDetach() override;
        void OnImGuiRender() override;

        void Begin();
        void End();
    private:
        float m_Time = 0.f;
    };

}
