#include "OrbitSystem2D.h"

OrbitSystem2D OrbitSystem2D::s_OrbitSystem2D;

static const LVM::BigFloat kGrav = { 6.6743f, -11 };


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
    for (auto node : m_Nodes)
    {
        // Integrate true anomaly
        auto& op = node->Parameters;
        op.TrueAnomaly += (float)dT * op.OSAMomentum / op.Position.SqrMagnitude(); // dTA / dT = h / r^2
        if (op.TrueAnomaly > LVM::PI2f)
        {
            op.TrueAnomaly -= LVM::PI2f;
        }
        node->NeedRecomputeState = true;
    }
}


void OrbitSystem2D::LoadLevel(const LVM::BigFloat& hostMass)
{
    m_LevelHost.reset(new OrbitTreeNode);
    m_LevelHost->Parameters.Gravitational = kGrav * hostMass;

    m_Nodes.clear();
}


uint32_t OrbitSystem2D::CreateOrbiter(const LVM::BigFloat& mass, const Limnova::Vector2& position, const Limnova::Vector2& velocity)
{
    OrbitTreeNode* newNode = new OrbitTreeNode;
    auto& op = newNode->Parameters;

    // TEMPORARY - applies to satellites of level host only
    op.Gravitational = m_LevelHost->Parameters.Gravitational;
    op.mu = op.Gravitational.GetFloat();
    LV_ASSERT((kGrav * mass).GetExponent() < op.Gravitational.GetExponent()
        - (int)cbrtf((float)op.Gravitational.GetExponent()), "Orbiter mass is too high!");

    op.Position = position;
    op.Velocity = velocity;
    newNode->NeedRecomputeState = false;
    ComputeElementsFromState(op);

    uint32_t idx = m_Nodes.size();
    m_Nodes.emplace_back(newNode);
    return idx;
}


uint32_t OrbitSystem2D::CreateOrbiter(const LVM::BigFloat& mass, const Limnova::Vector2& position, bool clockwise)
{
    float vMag = sqrtf(m_LevelHost->Parameters.Gravitational / sqrtf(position.SqrMagnitude()));
    Limnova::Vector2 vDir = clockwise ?
        Limnova::Vector2(position.y, -position.x).Normalized() :
        Limnova::Vector2(-position.y, position.x).Normalized();

    Limnova::Vector2 velocity = vMag * vDir;
    return CreateOrbiter(mass, position, velocity);
}


void OrbitSystem2D::ComputeElementsFromState(OrbitParameters& op)
{
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
        op.TrueAnomaly = LVM::PI2f - op.TrueAnomaly;
    }

    op.RightAscensionPeriapsis = acosf(op.BasisX.x);
    if (op.BasisX.y < 0)
    {
        op.TrueAnomaly = LVM::PI2f - op.TrueAnomaly;
    }

    float h2 = (float)pow(op.OSAMomentum, 2);
    op.h2mu = h2 / op.mu;
    op.muh = op.mu / op.OSAMomentum;

    float oneMinusE2 = (1.f - e2);
    op.SemiMajorAxis = op.h2mu / (op.mu * oneMinusE2);
    op.SemiMinorAxis = op.SemiMajorAxis * sqrtf(oneMinusE2);
    op.Centre = -op.SemiMajorAxis * op.Eccentricity * op.BasisX;

    op.Period = LVM::PI2f * op.SemiMajorAxis * op.SemiMinorAxis / op.OSAMomentum;
}


void OrbitSystem2D::ComputeStateVector(OrbitParameters& params)
{
    // TEMPORARY - circular only
    //op.Position.x = op.Periapsis * cos(op.TrueAnomaly);
    //op.Position.y = op.CcwF * op.Periapsis * sin(op.TrueAnomaly);

    // Ellipse
    float sinT = sin(params.TrueAnomaly);
    float cosT = cos(params.TrueAnomaly);
    params.Position = params.h2mu
        * (params.BasisX * cosT + params.BasisY * sinT)
        / (1.f + params.Eccentricity * cosT);

    params.Velocity = params.muh
        * (params.BasisY * (params.Eccentricity + cosT) - params.BasisX * sinT);
}


const OrbitSystem2D::OrbitParameters& OrbitSystem2D::GetParameters(const uint32_t orbiter)
{
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
    auto& op = m_Nodes[orbiter]->Parameters;

    op.TrueAnomaly = op.CcwF > 0 ? rightAscension - op.RightAscensionPeriapsis : op.RightAscensionPeriapsis - rightAscension;
    if (op.TrueAnomaly < 0)
    {
        op.TrueAnomaly += LVM::PI2f;
    }

    m_Nodes[orbiter]->NeedRecomputeState = true;
}
