#include "OrbitalLayer.h"


namespace Limnova
{

    OrbitalLayer::OrbitalLayer()
    {
    }


    void OrbitalLayer::OnAttach()
    {
        auto orbital0 = m_Scene.CreateEntity("Orbital 0");
        orbital0.AddComponent<OrbitalComponent>();

        auto orbital1 = m_Scene.CreateEntity("Orbital 1");
        orbital1.AddComponent<OrbitalComponent>();
    }


    void OrbitalLayer::OnDetach()
    {
    }


    void OrbitalLayer::OnUpdate(Timestep dT)
    {
        std::vector<Entity> orbitalEntities;
        m_Scene.GetEntitiesByComponents<OrbitalComponent>(orbitalEntities);
        for (auto entity : orbitalEntities)
        {
            auto orbital = entity.GetComponent<OrbitalComponent>();
        }
    }


    void OrbitalLayer::OnImGuiRender()
    {
    }


    void OrbitalLayer::OnEvent(Event& e)
    {
    }

}
