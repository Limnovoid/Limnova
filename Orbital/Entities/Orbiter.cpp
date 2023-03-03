#include "Orbiter.h"


Orbiter::Orbiter(const std::string& name, const float radius, const LV::Vector4& color, const OrbitalPhysics2D::NodeRef& nodeRef)
    : Entity(name), m_Radius(radius), m_Color(color), m_Node(nodeRef)
{
}


OrbRef Orbiter::Create(const std::string& name, const float radius, const LV::Vector4& color,
    const LV::BigFloat& mass, const InflOrbRef& initialHost, const LV::Vector2& scaledPosition, const LV::BigVector2& scaledVelocity)
{
    uint32_t id = OrbitalPhysics2D::Get().CreateOrbiterES(false, false, mass, initialHost->m_Node->GetId(), scaledPosition, scaledVelocity);
    return OrbRef(new Orbiter(name, radius, color, OrbitalPhysics2D::Get().GetNodeRef(id)));
}


OrbRef Orbiter::Create(const std::string& name, const float radius, const LV::Vector4& color,
    const LV::BigFloat& mass, const InflOrbRef& initialHost, const LV::Vector2& scaledPosition, const bool clockwise)
{
    uint32_t id = OrbitalPhysics2D::Get().CreateOrbiterCS(false, false, mass, initialHost->m_Node->GetId(), scaledPosition, clockwise);
    return OrbRef(new Orbiter(name, radius, color, OrbitalPhysics2D::Get().GetNodeRef(id)));
}


void Orbiter::OnUpdate(Limnova::Timestep dT)
{
}


void Orbiter::Destroy()
{
    OrbitalPhysics2D::Get().DestroyOrbiter(m_Node->GetId());

    Entity::Destroy();
}


//// InfluencingOrbiter ////////////////////////////////////////

InfluencingOrbiter::InfluencingOrbiter(const std::string& name, const float radius, const LV::Vector4& color, const OrbitalPhysics2D::InflRef& inflNodeRef)
    : Orbiter(name, radius, color, OrbitalPhysics2D::Get().GetNodeRef(inflNodeRef->GetId())), m_InflNode(inflNodeRef)
{
}


InfluencingOrbiter::~InfluencingOrbiter()
{
}


InflOrbRef InfluencingOrbiter::Create(const std::string& name, const float radius, const LV::Vector4& color,
    const LV::BigFloat& mass, const InflOrbRef& initialHost, const LV::Vector2& scaledPosition, const LV::BigVector2& scaledVelocity)
{
    uint32_t id = OrbitalPhysics2D::Get().CreateOrbiterES(true, false, mass, initialHost->m_Node->GetId(), scaledPosition, scaledVelocity);
    return InflOrbRef(new InfluencingOrbiter(name, radius, color, OrbitalPhysics2D::Get().GetInflRef(id)));
}


InflOrbRef InfluencingOrbiter::Create(const std::string& name, const float radius, const LV::Vector4& color,
    const LV::BigFloat& mass, const InflOrbRef& initialHost, const LV::Vector2& scaledPosition, const bool clockwise)
{
    uint32_t id = OrbitalPhysics2D::Get().CreateOrbiterCS(true, false, mass, initialHost->m_Node->GetId(), scaledPosition, clockwise);
    return InflOrbRef(new InfluencingOrbiter(name, radius, color, OrbitalPhysics2D::Get().GetInflRef(id)));
}


//// SystemHost ////////////////////////////////////////////////

SystemHost::SystemHost(const std::string& name, const float radius, const LV::Vector4& color, const OrbitalPhysics2D::InflRef& inflNodeRef)
    : InfluencingOrbiter(name, radius, color, inflNodeRef)
{
}


SystemHost::~SystemHost()
{
}


SystemRef SystemHost::Create(const std::string& name, const float radius, const LV::Vector4& color, const LV::BigFloat& mass, const Limnova::BigFloat& baseScaling)
{
    uint32_t id = OrbitalPhysics2D::Get().LoadLevel(mass, baseScaling);
    return SystemRef(new SystemHost(name, radius, color, OrbitalPhysics2D::Get().GetInflRef(id)));
}


//// PlayerShip ////////////////////////////////////////////////

Spacecraft::Spacecraft(const std::string& name, const float radius, const LV::Vector4& color, const OrbitalPhysics2D::NodeRef& nodeRef)
    : Orbiter(name, radius, color, nodeRef)
{
}


Spacecraft::~Spacecraft()
{
}


SpacecraftRef Spacecraft::Create(const std::string& name, const float radius, const LV::Vector4& color,
    const LV::BigFloat& mass, const InflOrbRef& initialHost, const LV::Vector2& scaledPosition, const LV::BigVector2& scaledVelocity)
{
    uint32_t id = OrbitalPhysics2D::Get().CreateOrbiterES(false, true, mass, initialHost->GetOrbitSystemId(), scaledPosition, scaledVelocity);
    return SpacecraftRef(new Spacecraft(name, radius, color, OrbitalPhysics2D::Get().GetNodeRef(id)));
}


SpacecraftRef Spacecraft::Create(const std::string& name, const float radius, const LV::Vector4& color,
    const LV::BigFloat& mass, const InflOrbRef& initialHost, const LV::Vector2& scaledPosition, const bool clockwise)
{
    uint32_t id = OrbitalPhysics2D::Get().CreateOrbiterCS(false, true, mass, initialHost->GetOrbitSystemId(), scaledPosition, clockwise);
    return SpacecraftRef(new Spacecraft(name, radius, color, OrbitalPhysics2D::Get().GetNodeRef(id)));
}


void Spacecraft::Accelerate(const LV::BigVector2& acceleration)
{
    OrbitalPhysics2D::Get().AccelerateOrbiter(m_Node->GetId(), acceleration);
}
