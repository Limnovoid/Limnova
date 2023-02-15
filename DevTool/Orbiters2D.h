#pragma once

#include <Limnova.h>
#include <Core/Layer.h>

#include "OrbitSystem2D.h"
#include "Entities/Orbiter.h"

namespace LV = Limnova;


class LIMNOVA_API Orbiters2D : public LV::Layer
{
public:
    Orbiters2D();
    ~Orbiters2D();

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(LV::Timestep dT) override;
    void OnImGuiRender() override;
    void OnEvent(LV::Event& e) override;
private:
    LV::Ref<LV::OrthographicPlanarCameraController> m_CameraController;

    LV::Ref<LV::Texture2D> m_CheckerboardTexture;
    LV::Ref<LV::Texture2D> m_CircleFillTexture;
    LV::Ref<LV::Texture2D> m_CircleTexture;
    LV::Ref<LV::Texture2D> m_CircleThickTexture;
    LV::Ref<LV::Texture2D> m_CircleLargeFillTexture;
    LV::Vector4 m_InfluenceColor = { 1.f, 0.8f, 0.2f, 0.25f };
    LV::Vector4 m_IntersectCircleColor = { 1.f, 0.3f, 0.2f, 0.5f };
    float m_Timescale = 0.1f;

    SystemRef m_SystemHost;
    std::unordered_map<uint32_t, OrbRef> m_Orbiters; // TEMPORARY - to be replaced by entities/components
    uint32_t m_CameraTrackingId = 0;
    uint32_t m_CameraHostId = 0;
    uint32_t m_CameraRelativeLevel = 0;
    bool m_CameraTrackingChanged = false;
    PlayerRef m_PlayerShip;
};
