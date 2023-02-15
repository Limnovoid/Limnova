#include "Orbiter.h"


Orbiter::Orbiter(const std::string& name, const float radius, const LV::Vector4& color, const OrbitSystem2D::NodeRef& nodeRef)
    : Entity(name), m_Radius(radius), m_Color(color), m_Node(nodeRef)
{
}


void Orbiter::OnUpdate(Limnova::Timestep dT)
{
}


void Orbiter::Destroy()
{
    OrbitSystem2D::Get().DestroyOrbiter(m_Node->GetId());

    Entity::Destroy();
}


OrbRef Orbiter::Create(const std::string& name, const float radius, const LV::Vector4& color,
    const LV::BigFloat& mass, const InflOrbRef& initialHost, const LV::Vector2& scaledPosition, const LV::BigVector2& scaledVelocity)
{
    uint32_t id = OrbitSystem2D::Get().CreateOrbiterES(false, false, mass, initialHost->m_Node->GetId(), scaledPosition, scaledVelocity);
    return OrbRef(new Orbiter(name, radius, color, OrbitSystem2D::Get().GetNodeRef(id)));
}


OrbRef Orbiter::Create(const std::string& name, const float radius, const LV::Vector4& color,
    const LV::BigFloat& mass, const InflOrbRef& initialHost, const LV::Vector2& scaledPosition, const bool clockwise)
{
    uint32_t id = OrbitSystem2D::Get().CreateOrbiterCS(false, false, mass, initialHost->m_Node->GetId(), scaledPosition, clockwise);
    return OrbRef(new Orbiter(name, radius, color, OrbitSystem2D::Get().GetNodeRef(id)));
}


//// InfluencingOrbiter ////////////////////////////////////////

InfluencingOrbiter::InfluencingOrbiter(const std::string& name, const float radius, const LV::Vector4& color, const OrbitSystem2D::InflRef& inflNodeRef)
    : Orbiter(name, radius, color, OrbitSystem2D::Get().GetNodeRef(inflNodeRef->GetId())), m_InflNode(inflNodeRef)
{
}


InfluencingOrbiter::~InfluencingOrbiter()
{
}


InflOrbRef InfluencingOrbiter::Create(const std::string& name, const float radius, const LV::Vector4& color,
    const LV::BigFloat& mass, const InflOrbRef& initialHost, const LV::Vector2& scaledPosition, const LV::BigVector2& scaledVelocity)
{
    uint32_t id = OrbitSystem2D::Get().CreateOrbiterES(true, false, mass, initialHost->m_Node->GetId(), scaledPosition, scaledVelocity);
    return InflOrbRef(new InfluencingOrbiter(name, radius, color, OrbitSystem2D::Get().GetInflRef(id)));
}


InflOrbRef InfluencingOrbiter::Create(const std::string& name, const float radius, const LV::Vector4& color,
    const LV::BigFloat& mass, const InflOrbRef& initialHost, const LV::Vector2& scaledPosition, const bool clockwise)
{
    uint32_t id = OrbitSystem2D::Get().CreateOrbiterCS(true, false, mass, initialHost->m_Node->GetId(), scaledPosition, clockwise);
    return InflOrbRef(new InfluencingOrbiter(name, radius, color, OrbitSystem2D::Get().GetInflRef(id)));
}


//// SystemHost ////////////////////////////////////////////////

SystemHost::SystemHost(const std::string& name, const float radius, const LV::Vector4& color, const OrbitSystem2D::InflRef& inflNodeRef)
    : InfluencingOrbiter(name, radius, color, inflNodeRef)
{
}


SystemHost::~SystemHost()
{
}


SystemRef SystemHost::Create(const std::string& name, const float radius, const LV::Vector4& color, const LV::BigFloat& mass, const Limnova::BigFloat& baseScaling)
{
    uint32_t id = OrbitSystem2D::Get().LoadLevel(mass, baseScaling);
    return SystemRef(new SystemHost(name, radius, color, OrbitSystem2D::Get().GetInflRef(id)));
}


//// PlayerShip ////////////////////////////////////////////////

PlayerShip::PlayerShip(const std::string& name, const float radius, const LV::Vector4& color, const OrbitSystem2D::NodeRef& nodeRef)
    : Orbiter(name, radius, color, nodeRef)
{
}


PlayerShip::~PlayerShip()
{
}


PlayerRef PlayerShip::Create(const std::string& name, const float radius, const LV::Vector4& color,
    const LV::BigFloat& mass, const InflOrbRef& initialHost, const LV::Vector2& scaledPosition, const LV::BigVector2& scaledVelocity)
{
    uint32_t id = OrbitSystem2D::Get().CreateOrbiterES(true, false, mass, initialHost->GetOrbitSystemId(), scaledPosition, scaledVelocity);
    return std::shared_ptr<PlayerShip>(new PlayerShip(name, radius, color, OrbitSystem2D::Get().GetNodeRef(id)));
}


PlayerRef PlayerShip::Create(const std::string& name, const float radius, const LV::Vector4& color,
    const LV::BigFloat& mass, const InflOrbRef& initialHost, const LV::Vector2& scaledPosition, const bool clockwise)
{
    uint32_t id = OrbitSystem2D::Get().CreateOrbiterCS(true, false, mass, initialHost->GetOrbitSystemId(), scaledPosition, clockwise);
    return PlayerRef(new PlayerShip(name, radius, color, OrbitSystem2D::Get().GetNodeRef(id)));
}
