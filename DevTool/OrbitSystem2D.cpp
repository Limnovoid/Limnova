#include "OrbitSystem2D.h"


static const Limnova::BigFloat kGrav = { 6.6743f, -11 };


OrbitSystem2D OrbitSystem2D::s_OrbitSystem2D;

void OrbitSystem2D::Init()
{
    s_OrbitSystem2D.m_LevelHost.reset();
    s_OrbitSystem2D.m_InflNodes.clear();
}


OrbitSystem2D& OrbitSystem2D::Get()
{
    return s_OrbitSystem2D;
}


void OrbitSystem2D::Update(Limnova::Timestep dT)
{
    LV_PROFILE_FUNCTION();

    for (uint32_t n = 1; n < m_InflNodes.size(); n++)
    {
        // Integrate true anomaly
        auto& op = m_InflNodes[n]->Parameters;
        op.TrueAnomaly += (float)dT * m_Timescale * op.OSAMomentum / op.Position.SqrMagnitude(); // dTA / dT = h / r^2
        if (op.TrueAnomaly > Limnova::PI2f)
        {
            op.TrueAnomaly -= Limnova::PI2f;
        }
        m_InflNodes[n]->NeedRecomputeState = true;
    }
}


void OrbitSystem2D::LoadLevel(const Limnova::BigFloat& hostMass, const Limnova::BigFloat& baseScaling)
{
    LV_PROFILE_FUNCTION();

    m_LevelHost.reset(new InfluencingNode);
    m_LevelHost->Id = 0;
    m_LevelHost->Influence.TotalScaling = baseScaling;
    m_LevelHost->Parameters.GravAsOrbiter = kGrav * hostMass;

    // Length dimension in G (the gravitational constant) is cubed - scaling must be cubed when computing scaled-GM
    m_LevelHost->Parameters.GravAsHost = m_LevelHost->Parameters.GravAsOrbiter / Limnova::BigFloat::Pow(baseScaling, 3);

    m_InflNodes.clear();
    m_InflNodes.push_back(m_LevelHost);
}


uint32_t OrbitSystem2D::CreateOrbiter(const Limnova::BigFloat& mass, const Limnova::Vector2& position, const Limnova::Vector2& velocity)
{
    LV_PROFILE_FUNCTION();

    // Determine parent node (host of orbit)
    auto& p = FindLowestOverlappingInfluence(position);

    Limnova::Vector2 scaledPosition = (position - p->Parameters.Position) * p->Influence.TotalScaling.GetFloat();
    Limnova::Vector2 scaledVelocity = (velocity - p->Parameters.Velocity) * p->Influence.TotalScaling.GetFloat();

    return CreateInfluencingNode(p, mass, scaledPosition, scaledVelocity);
}


uint32_t OrbitSystem2D::CreateOrbiter(const Limnova::BigFloat& mass, const Limnova::Vector2& position, bool clockwise)
{
    LV_PROFILE_FUNCTION();

    // Determine parent node (host of orbit)
    auto& p = FindLowestOverlappingInfluence(position);

    // Compute relative velocity of circular orbit
    Limnova::Vector2 scaledPosition = (position - p->Parameters.Position) * p->Influence.TotalScaling.GetFloat();
    float vMag = sqrtf(p->Parameters.GravAsHost.GetFloat() / sqrtf(scaledPosition.SqrMagnitude())); // TEMPORARY - use BigFloat
    Limnova::Vector2 vDir = clockwise ?
        Limnova::Vector2(scaledPosition.y, -scaledPosition.x).Normalized() :
        Limnova::Vector2(-scaledPosition.y, scaledPosition.x).Normalized();

    return CreateInfluencingNode(p, mass, scaledPosition, vMag * vDir);
}


uint32_t OrbitSystem2D::CreateInfluencingNode(const InflRef& parent, const Limnova::BigFloat& mass, const Limnova::Vector2& scaledPosition, const Limnova::Vector2& scaledVelocity)
{
    LV_PROFILE_FUNCTION();

    InfluencingNode* newNode = new InfluencingNode;
    newNode->Parent = parent;

    // Compute gravitational properties of system
    auto& op = newNode->Parameters;
    op.GravAsOrbiter = newNode->Parent->Parameters.GravAsHost; // mu = GM -> Assumes mass of orbiter is insignificant compared to host
    op.mu = op.GravAsOrbiter.GetFloat(); // TEMPORARY - realistic values are too small for floats!

    // Compute orbital elements
    op.Position = scaledPosition;
    op.Velocity = scaledVelocity;
    newNode->NeedRecomputeState = false;
    ComputeElementsFromState(op);

    // Compute this orbiter's influence
    ComputeInfluence(newNode, parent, mass);

    // Add node to tree
    newNode->Id = m_InflNodes.size();
    newNode->Parent->InfluencingChildren.push_back(m_InflNodes.emplace_back(newNode));
    return newNode->Id;
}


void OrbitSystem2D::ComputeElementsFromState(OrbitParameters& op)
{
    LV_PROFILE_FUNCTION();
    // Some of these computations use optimisations which only apply
    // to orbits in the XY plane: assume the physics/maths used is
    // suitable only for 2D simulations!

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
        op.RightAscensionPeriapsis = Limnova::PI2f - op.RightAscensionPeriapsis;
    }

    float h2 = (float)pow(op.OSAMomentum, 2);
    op.h2mu = h2 / op.mu;
    op.muh = op.mu / op.OSAMomentum;

    float oneMinusE2 = (1.f - e2);
    op.SemiMajorAxis = h2 / (op.mu * oneMinusE2);
    op.SemiMinorAxis = op.SemiMajorAxis * sqrtf(oneMinusE2);
    op.Centre = -op.SemiMajorAxis * op.Eccentricity * op.BasisX;

    op.Period = Limnova::PI2f * op.SemiMajorAxis * op.SemiMinorAxis / op.OSAMomentum;
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


void OrbitSystem2D::ComputeInfluence(InfluencingNode* influencingNode, const InflRef& parent, const Limnova::BigFloat& mass)
{
    LV_PROFILE_FUNCTION();

    auto& op = influencingNode->Parameters;
    auto& infl = influencingNode->Influence;

    Limnova::BigFloat parentScaledGrav = kGrav * mass * Limnova::BigFloat::Pow(parent->Influence.TotalScaling, 3);
#ifdef LV_DEBUG
    auto& hp = parent->Parameters;
    if (parentScaledGrav.GetExponent() > hp.GravAsHost.GetExponent()
        - (int)cbrtf((float)hp.GravAsHost.GetExponent()) - 1)
    {
        LV_CORE_ERROR("Orbiter mass ({0}) is too high for lowest overlapping circle of influence (orbiter {1})!", mass, parent->Id);
        delete influencingNode;
        LV_CORE_ASSERT(false, "");
    }
#endif
    infl.Radius = op.SemiMajorAxis * pow((parentScaledGrav / op.GravAsOrbiter).GetFloat(), 0.4f);
    infl.TotalScaling = parent->Influence.TotalScaling / Limnova::BigFloat(infl.Radius);
    op.GravAsHost = kGrav * mass * Limnova::BigFloat::Pow(Limnova::BigFloat(infl.TotalScaling), 3); // G's length dimension is cubed - scaling must be cubed: scaled-GM = GM / scale^3
}


OrbitSystem2D::InflRef& OrbitSystem2D::FindLowestOverlappingInfluence(const Limnova::Vector2& absolutePosition)
{
    LV_PROFILE_FUNCTION();

    uint32_t parentId = 0, inflNodeId;
    Limnova::Vector2 scaledPosition = absolutePosition * m_LevelHost->Influence.TotalScaling.GetFloat();
    for (uint32_t i = 0; i < m_InflNodes.size(); i++)
    {
        auto& inflNode = FindOverlappingChildInfluence(m_InflNodes[parentId], scaledPosition);
        if (parentId == inflNode->Id)
        {
            return inflNode;
        }
        scaledPosition = (scaledPosition - inflNode->Parameters.Position) / inflNode->Influence.Radius;
        parentId = inflNode->Id;
    }
    LV_CORE_ASSERT(false, "Function should never reach this line!");
    return m_LevelHost;
}


OrbitSystem2D::InflRef& OrbitSystem2D::FindOverlappingChildInfluence(InflRef& parent, const Limnova::Vector2& scaledPosition)
{
    LV_PROFILE_FUNCTION();

    for (auto& child : parent->InfluencingChildren)
    {
        float separation = sqrt((scaledPosition - child->Parameters.Position).SqrMagnitude());
        if (separation < child->Influence.Radius)
        {
            return child;
        }
    }
    return parent;
}


const OrbitSystem2D::OrbitParameters& OrbitSystem2D::GetParameters(const uint32_t orbiter)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(orbiter < m_InflNodes.size() && orbiter >= 0, "Invalid orbiter ID!");

    auto& node = m_InflNodes[orbiter];
    if (node->NeedRecomputeState)
    {
        ComputeStateVector(node->Parameters);
        node->NeedRecomputeState = false;
    }
    return m_InflNodes[orbiter]->Parameters;
}


float OrbitSystem2D::GetRadiusOfInfluence(const uint32_t orbiter)
{
    LV_CORE_ASSERT(orbiter < m_InflNodes.size() && orbiter >= 0, "Invalid orbiter ID!");

    return m_InflNodes[orbiter]->Influence.Radius;
}


float OrbitSystem2D::GetScaling(const uint32_t host)
{
    LV_CORE_ASSERT(host < m_InflNodes.size() && host >= 0, "Invalid orbiter ID!");

    return m_InflNodes[host]->Influence.TotalScaling.GetFloat();
}


void OrbitSystem2D::GetChildren(const uint32_t host, std::vector<uint32_t>& ids)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(host < m_InflNodes.size() && host >= 0, "Invalid orbiter ID!");

    for (auto& child : m_InflNodes[host]->InfluencingChildren)
    {
        ids.push_back(child->Id);
    }
}


void OrbitSystem2D::SetOrbiterRightAscension(const uint32_t orbiter, const float rightAscension)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(orbiter > 0, "Cannot set right ascension of level host!");

    auto& op = m_InflNodes[orbiter]->Parameters;

    op.TrueAnomaly = op.CcwF > 0 ? rightAscension - op.RightAscensionPeriapsis : op.RightAscensionPeriapsis - rightAscension;
    if (op.TrueAnomaly < 0)
    {
        op.TrueAnomaly += Limnova::PI2f;
    }

    m_InflNodes[orbiter]->NeedRecomputeState = true;
}


void OrbitSystem2D::GetAllHosts(std::vector<uint32_t>& ids)
{
    LV_PROFILE_FUNCTION();

    for (auto& infl : m_InflNodes)
    {
        ids.push_back(infl->Id);
    }
}
