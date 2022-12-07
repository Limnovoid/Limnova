#pragma once

#include <Limnova.h>
#include <Core/Layer.h>

#include "Orbiter.h"


class LIMNOVA_API Orbiters2D : public Limnova::Layer
{
public:
    Orbiters2D();
    ~Orbiters2D();

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(Limnova::Timestep dT) override;
    void OnImGuiRender() override;
    void OnEvent(Limnova::Event& e) override;
private:
    Limnova::Ref<Limnova::OrthographicPlanarCameraController> m_CameraController;

    Limnova::Ref<Limnova::Texture2D> m_CheckerboardTexture;
    Limnova::Ref<Limnova::Texture2D> m_CircleFillTexture;
    Limnova::Ref<Limnova::Texture2D> m_CircleTexture;
    Limnova::Vector4 m_InfluenceColor = { 1.f, 0.8f, 0.2f, 0.25f };
    float m_Timescale = 1.f;

    uint32_t m_Orb0Id, m_Orb1Id;
    Limnova::Vector4 m_Orb0Color = { 1.f, 0.3f, 0.2f, 1.f };
    Limnova::Vector4 m_Orb1Color = { 0.2f, 0.3f, 1.f, 1.f };
    uint32_t m_CameraTrackingId = std::numeric_limits<uint32_t>::max();
    Limnova::Vector2 m_CameraTrackingPosition;
    bool m_CameraTrackingChanged = false;
};
