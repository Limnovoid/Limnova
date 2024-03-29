#pragma once

#include "Core.h"
#include "Core/Timestep.h"
#include "Events/Event.h"


namespace Limnova
{

    class LIMNOVA_API Layer
    {
    public:
        Layer(const std::string& name = "Layer");
        virtual ~Layer();

        virtual void OnAttach() {}
        virtual void OnDetach() {}
        virtual void OnUpdate(Timestep dT) {}
        virtual void OnImGuiRender() {}
        virtual void OnEvent(Event& e) {}

        inline const std::string& GetName() const { return m_DebugName; }
    protected:
        std::string m_DebugName;
    };

}