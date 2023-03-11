#pragma once

#include "OrbitalScene.h"

#include <Limnova.h>


namespace Limnova
{

    class OrbitalLayer : public Layer
    {
    public:
        OrbitalLayer();
        ~OrbitalLayer() = default;

        void OnAttach() override;
        void OnDetach() override;

        void OnUpdate(Timestep dT) override;
        void OnImGuiRender() override;
        void OnEvent(Event& e) override;
    private:
        OrbitalScene m_Scene;
    };

}
