#include "OrbitSystem2D.h"

OrbitSystem2D OrbitSystem2D::s_OrbitSystem2D;

static const Limnova::BigFloat kGrav = { 6.6743f, -11 };


void OrbitSystem2D::Init()
{
    s_OrbitSystem2D.m_LevelHost.reset();
    s_OrbitSystem2D.m_Nodes.clear();
}


OrbitSystem2D& OrbitSystem2D::Get()
{
    return s_OrbitSystem2D;
}


void OrbitSystem2D::Update(Limnova::Timestep dT)
{
    LV_PROFILE_FUNCTION();

    for (auto& node : m_Nodes)
    {
        // Integrate true anomaly
        auto& op = node->Parameters;
        op.TrueAnomaly += (float)dT * m_Timescale * op.OSAMomentum / op.Position.SqrMagnitude(); // dTA / dT = h / r^2
        if (op.TrueAnomaly > Limnova::PI2f)
        {
            op.TrueAnomaly -= Limnova::PI2f;
        }
        node->NeedRecomputeState = true;
    }
}


void OrbitSystem2D::LoadLevel(const Limnova::BigFloat& hostMass)
{
    LV_PROFILE_FUNCTION();

    m_LevelHost.reset(new OrbitTreeNode);
    m_LevelHost->Parameters.MassGrav = kGrav * hostMass;
    m_LevelHost->Parameters.RadiusOfInfluence = 1.1f;

    m_Nodes.clear();
}


uint32_t OrbitSystem2D::CreateOrbiter(const Limnova::BigFloat& mass, const Limnova::Vector2& position, const Limnova::Vector2& velocity)
{
    LV_PROFILE_FUNCTION();

    // Determine parent node (host of orbit)
    auto& p = FindLowestOverlappingCOI(position);

    return CreateOrbiterImpl(p, mass, position, velocity);
}


uint32_t OrbitSystem2D::CreateOrbiter(const Limnova::BigFloat& mass, const Limnova::Vector2& position, bool clockwise)
{
    LV_PROFILE_FUNCTION();

    // Determine parent node (host of orbit)
    auto& p = FindLowestOverlappingCOI(position);

    // Compute velocity of circular orbit
    float vMag = sqrtf(p->Parameters.MassGrav.GetFloat() / sqrtf(position.SqrMagnitude()));
    Limnova::Vector2 vDir = clockwise ?
        Limnova::Vector2(position.y, -position.x).Normalized() :
        Limnova::Vector2(-position.y, position.x).Normalized();

    Limnova::Vector2 velocity = vMag * vDir;
    return CreateOrbiterImpl(p, mass, position, velocity);
}


uint32_t OrbitSystem2D::CreateOrbiterImpl(const NodeRef& parent, const Limnova::BigFloat& mass, const Limnova::Vector2& position, const Limnova::Vector2& velocity)
{
    LV_PROFILE_FUNCTION();

    OrbitTreeNode* newNode = new OrbitTreeNode;
    newNode->Parent = parent;

    // Compute gravitational properties of system
    auto& op = newNode->Parameters;
    op.MassGrav = kGrav * mass;
#ifdef LV_DEBUG
    auto& hp = newNode->Parent->Parameters;
    if (op.MassGrav.GetExponent() > hp.MassGrav.GetExponent()
        - (int)cbrtf((float)hp.MassGrav.GetExponent()) - 1)
    {
        LV_CORE_ERROR("Orbiter mass ({0}) is too high for lowest overlapping circle of influence (orbiter {1})!", mass, newNode->Parent->Id);
        delete newNode;
        return std::numeric_limits<uint32_t>::max();
    }
#endif
    op.Gravitational = newNode->Parent->Parameters.MassGrav; // mu = GM -> Assumes mass of orbiter is insignificant compared to host    
    op.mu = op.Gravitational.GetFloat(); // TEMPORARY - realistic values are too large for floats!

    // Compute orbital elements
    op.Position = position;
    op.Velocity = velocity;
    newNode->NeedRecomputeState = false;
    ComputeElementsFromState(op);

    // Add node to tree
    newNode->Id = m_Nodes.size();
    newNode->Parent->Children.push_back(m_Nodes.emplace_back(newNode));
    return newNode->Id;
}


void OrbitSystem2D::ComputeElementsFromState(OrbitParameters& op)
{
    LV_PROFILE_FUNCTION();
    // Some of these computations use optimisations which only apply
    // to orbits in the XY plane: the mechanics in this function is 
    // suitable only for 2D orbit simulations!

    float signedH = op.Position.x * op.Velocity.y - op.Position.y * op.Velocity.x;
    int ccwF = signedH < 0 ? -1.f : 1.f;
    op.OSAMomentum = abs(signedH);

    Limnova::Vector2 ur = op.Position.Normalized();
    Limnova::Vector2 eVec = Limnova::Vector2(
        op.Velocity.y * signedH, -op.Velocity.x * signedH) / op.mu - ur;
    float e2 = eVec.SqrMagnitude();
    if (e2 > 0)
    {
        // Elliptical
        op.Eccentricity = sqrtf(e2);
        op.BasisX = eVec.Normalized();
    }
    else
    {
        // Circular
        op.Eccentricity = 0;
        op.BasisX = { 1.f, 0.f };
    }
    Limnova::Vector2 cwHorz = { -op.Position.y, op.Position.x };
    op.BasisY = ccwF * Limnova::Vector2(- op.BasisX.y, op.BasisX.x);

    op.TrueAnomaly = acosf(op.BasisX.Dot(ur));
    if (op.Velocity.Dot(ur) < 0)
    {
        op.TrueAnomaly = Limnova::PI2f - op.TrueAnomaly;
    }

    op.RightAscensionPeriapsis = acosf(op.BasisX.x);
    if (op.BasisX.y < 0)
    {
        op.TrueAnomaly = Limnova::PI2f - op.TrueAnomaly;
    }

    float h2 = (float)pow(op.OSAMomentum, 2);
    op.h2mu = h2 / op.mu;
    op.muh = op.mu / op.OSAMomentum;

    float oneMinusE2 = (1.f - e2);
    op.SemiMajorAxis = op.h2mu / (op.mu * oneMinusE2);
    op.SemiMinorAxis = op.SemiMajorAxis * sqrtf(oneMinusE2);
    op.Centre = -op.SemiMajorAxis * op.Eccentricity * op.BasisX;

    op.Period = Limnova::PI2f * op.SemiMajorAxis * op.SemiMinorAxis / op.OSAMomentum;

    op.RadiusOfInfluence = op.SemiMajorAxis * pow((op.MassGrav / op.Gravitational).GetFloat(), 0.4f);
}


void OrbitSystem2D::ComputeStateVector(OrbitParameters& params)
{
    LV_PROFILE_FUNCTION();

    float sinT = sin(params.TrueAnomaly);
    float cosT = cos(params.TrueAnomaly);
    params.Position = params.h2mu
        * (params.BasisX * cosT + params.BasisY * sinT)
        / (1.f + params.Eccentricity * cosT);

    params.Velocity = params.muh
        * (params.BasisY * (params.Eccentricity + cosT) - params.BasisX * sinT);
}


OrbitSystem2D::NodeRef& OrbitSystem2D::FindLowestOverlappingCOI(const Limnova::Vector2& position)
{
    LV_PROFILE_FUNCTION();

    uint32_t id = std::numeric_limits<uint32_t>::max();
    OrbitTreeNode* node = m_LevelHost.get();
    bool seek = true;
    while (seek)
    {
        seek = false;

        for (auto& child : node->Children)
        {
            float separation = sqrt((position - child->Parameters.Position).SqrMagnitude());
            if (separation < child->Parameters.RadiusOfInfluence)
            {
                id = child->Id;
                node = child.get();
                bool seek = true;
                break;
            }
        }
    }
    return id == std::numeric_limits<uint32_t>::max()
        ? m_LevelHost : m_Nodes[id];
}


const OrbitSystem2D::OrbitParameters& OrbitSystem2D::GetParameters(const uint32_t orbiter)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(orbiter < m_Nodes.size() && orbiter >= 0, "Invalid orbiter ID!");

    auto& node = m_Nodes[orbiter];
    if (node->NeedRecomputeState)
    {
        ComputeStateVector(node->Parameters);
        node->NeedRecomputeState = false;
    }
    return m_Nodes[orbiter]->Parameters;
}


const OrbitSystem2D::OrbitParameters& OrbitSystem2D::GetHostRenderInfo()
{
    return m_LevelHost->Parameters;
}


void OrbitSystem2D::SetOrbiterRightAscension(const uint32_t orbiter, const float rightAscension)
{
    LV_PROFILE_FUNCTION();

    auto& op = m_Nodes[orbiter]->Parameters;

    op.TrueAnomaly = op.CcwF > 0 ? rightAscension - op.RightAscensionPeriapsis : op.RightAscensionPeriapsis - rightAscension;
    if (op.TrueAnomaly < 0)
    {
        op.TrueAnomaly += Limnova::PI2f;
    }

    m_Nodes[orbiter]->NeedRecomputeState = true;
}
