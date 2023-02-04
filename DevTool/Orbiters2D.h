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
    Limnova::Ref<Limnova::Texture2D> m_CircleThickTexture;
    Limnova::Vector4 m_InfluenceColor = { 1.f, 0.8f, 0.2f, 0.25f };
    Limnova::Vector4 m_IntersectCircleColor = { 1.f, 0.3f, 0.2f, 0.5f };
    float m_Timescale = 0.1f;

    struct OrbiterRenderInfo
    {
        std::string Name;
        float Radius;
        Limnova::Vector4 Color;
    };
    std::unordered_map<uint32_t, OrbiterRenderInfo>m_OrbiterRenderInfo; // TEMPORARY - to be replaced by entities/components
    uint32_t m_CameraTrackingId = 0;
    uint32_t m_CameraHostId = 0;
    bool m_CameraTrackingChanged = false;
};
