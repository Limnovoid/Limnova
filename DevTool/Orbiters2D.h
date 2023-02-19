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
    LV::Vector4 m_InfluenceColor = { 1.f, 0.7f, 0.2f, 0.25f };
    LV::Vector4 m_IntersectCircleColor = { 1.f, 0.3f, 0.2f, 0.5f };
    float m_Timescale = 0.1f;

    SystemRef m_SystemHost;
    std::unordered_map<uint32_t, OrbRef> m_Orbiters; // TEMPORARY - to be replaced by entities/components
    uint32_t m_CameraTrackingId = 0;
    uint32_t m_CameraRelativeLevel = 1;
    SpacecraftRef m_PlayerShip;

    bool m_ZoomingIntoSystem = false;
    bool m_ZoomingOutOfSystem = false;

    struct InputInfo
    {
        LV::Vector2 MouseScenePosition;

        bool ShipIsBeingControlled = false;
        bool ShipIsThrusting = false;

        bool WeaponSelected = false;
        OrbitSystem2D::OrbitParameters TargetingOrbit;
    } m_Input;

    struct OrbiterUI
    {
        bool IsHovered = false;
        std::vector<uint32_t> SubOrbiters;
    };

    //class Projectile
    //{
    //public:
    //    Projectile(const OrbRef& launcher, const InflOrbRef& launcherHost, const OrbRef& target, const LV::Vector2& scaledLaunchVelocity);
    //    ~Projectile() {}
    //
    //    const OrbRef& GetRef() { return m_OrbRef; }
    //private:
    //    OrbRef m_OrbRef;
    //private:
    //    static constexpr float s_Radius = 0.012f;
    //    static const LV::Vector4 s_Color;
    //    static const LV::BigFloat s_Mass;
    //};
    //std::unordered_map<uint32_t, Projectile> m_Projectiles;
private:
    void GetCameraTrackingIds(uint32_t* sceneHostId, uint32_t* cameraTrackingId);
    bool PlayerShipIsVisible(const uint32_t sceneHostId, const uint32_t sceneTrackingId);

    bool OnMouseScrolled(LV::MouseScrolledEvent& e);
    bool OnMouseButtonPressed(LV::MouseButtonPressedEvent& event);
    bool OnKeyPressed(LV::KeyPressedEvent& e);
};
