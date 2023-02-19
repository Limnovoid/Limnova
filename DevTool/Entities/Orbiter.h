#pragma once

#include <Limnova.h>

#include "Entity.h"
#include "OrbitSystem2D.h"

namespace LV = Limnova;


class Orbiter;
using OrbRef = std::shared_ptr<Orbiter>;

class InfluencingOrbiter;
using InflOrbRef = std::shared_ptr<InfluencingOrbiter>;

class Orbiter : public Entity
{
protected:
    Orbiter(const std::string& name, const float radius, const LV::Vector4& color, const OrbitSystem2D::NodeRef& nodeRef);
public:
    virtual ~Orbiter() {}

    // Create a non-influencing static orbiter - specify initial position and velocity.
    static OrbRef Create(const std::string& name, const float radius, const LV::Vector4& color,
        const LV::BigFloat& mass, const InflOrbRef& initialHost, const LV::Vector2& scaledPosition, const LV::BigVector2& scaledVelocity);

    // Create a non-influencing static orbiter - specify initial position and orientation of circular orbit.
    static OrbRef Create(const std::string& name, const float radius, const LV::Vector4& color,
        const LV::BigFloat& mass, const InflOrbRef& initialHost, const LV::Vector2& scaledPosition, const bool clockwise);
public:
    void OnUpdate(Limnova::Timestep dT) override;

    void Destroy();

    float GetRadius() { return m_Radius; }
    LV::Vector4 GetColor() { return m_Color; }

    uint32_t GetOrbitSystemId() { return m_Node->GetId(); }
    uint32_t GetHostOrbitSystemId() { return m_Node->GetHost(); }
    const OrbitSystem2D::OrbitParameters& GetParameters() { return m_Node->GetParameters(); }

    void SetRadius(const float radius) { m_Radius = radius; }
    void SetColor(const LV::Vector4& color) { m_Color = color; }
protected:
    float m_Radius;
    LV::Vector4 m_Color;
    OrbitSystem2D::NodeRef m_Node;
protected:
    const OrbitSystem2D::NodeRef& GetNode() { return m_Node; }
};


class InfluencingOrbiter : public Orbiter
{
protected:
    InfluencingOrbiter(const std::string& name, const float radius, const LV::Vector4& color, const OrbitSystem2D::InflRef& inflNodeRef);
public:
    ~InfluencingOrbiter();

    // Create an influencing static orbiter - specify initial position and velocity.
    static InflOrbRef Create(const std::string& name, const float radius, const LV::Vector4& color,
        const LV::BigFloat& mass, const InflOrbRef& initialHost, const LV::Vector2& scaledPosition, const LV::BigVector2& scaledVelocity);

    // Create an influencing static orbiter - specify initial position and orientation of circular orbit.
    static InflOrbRef Create(const std::string& name, const float radius, const LV::Vector4& color,
        const LV::BigFloat& mass, const InflOrbRef& initialHost, const LV::Vector2& scaledPosition, const bool clockwise);
protected:
    OrbitSystem2D::InflRef m_InflNode;
};


class SystemHost;
using SystemRef = std::shared_ptr<SystemHost>;

class SystemHost : public InfluencingOrbiter
{
private:
    SystemHost(const std::string& name, const float radius, const LV::Vector4& color, const OrbitSystem2D::InflRef& inflNodeRef);
public:
    ~SystemHost();

    // Create/load a new orbit system - unloads the previous system and invalidates the objects belonging to it.
    static SystemRef Create(const std::string& name, const float radius, const LV::Vector4& color,
        const LV::BigFloat& mass, const Limnova::BigFloat& baseScaling);
};


class Spacecraft;
using SpacecraftRef = std::shared_ptr<Spacecraft>;

class Spacecraft : public Orbiter
{
protected:
    Spacecraft(const std::string& name, const float radius, const LV::Vector4& color, const OrbitSystem2D::NodeRef& nodeRef);
public:
    ~Spacecraft();

    // Create a non-influencing dynamic orbiter with controllable self-acceleration - specify initial position and velocity.
    static SpacecraftRef Create(const std::string& name, const float radius, const LV::Vector4& color,
        const LV::BigFloat& mass, const InflOrbRef& initialHost, const LV::Vector2& scaledPosition, const LV::BigVector2& scaledVelocity);

    // Create a non-influencing dynamic orbiter with controllable self-acceleration - specify initial position and orientation of circular orbit.
    static SpacecraftRef Create(const std::string& name, const float radius, const LV::Vector4& color,
        const LV::BigFloat& mass, const InflOrbRef& initialHost, const LV::Vector2& scaledPosition, const bool clockwise);
public:
    void Accelerate(const LV::BigVector2& acceleration);
};
