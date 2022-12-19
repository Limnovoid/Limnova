#include "OrbitSystem2D.h"

#include <chrono>


namespace LV = Limnova;

static const LV::BigFloat kGrav = { 6.6743f, -11 };


OrbitSystem2D OrbitSystem2D::s_OrbitSystem2D;

void OrbitSystem2D::Init()
{
    s_OrbitSystem2D.m_LevelHost.reset();
    s_OrbitSystem2D.m_InflNodes.clear();


    // debug - orbiter integration accuracy
    s_OrbitSystem2D.m_Data.clear();
    s_OrbitSystem2D.m_ActualPeriods.clear();
}


OrbitSystem2D& OrbitSystem2D::Get()
{
    return s_OrbitSystem2D;
}


void OrbitSystem2D::Shutdown()
{
    std::ofstream of;
    const auto tp_sec = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
    const auto tp_day = std::chrono::floor<std::chrono::days>(tp_sec);
    const std::chrono::year_month_day ymd(tp_day);
    const std::chrono::hh_mm_ss hms(tp_sec - tp_day);
    for (uint32_t n = 1; n < s_OrbitSystem2D.m_InflNodes.size(); n++)
    {
        of.open(s_OrbitSystem2D.m_Filenames[n], std::ofstream::app);
        if (!of.is_open())
        {
            LV_CORE_ERROR("Could not open debug data file '{0}'!", s_OrbitSystem2D.m_Filenames[n]);
            continue;
        }

        of  << std::format("{:%F}", ymd) << " | " << std::format("{:%T}", hms) << '\n'
            << s_OrbitSystem2D.m_Data[n].str()
            << std::endl;

        of.close();
    }
}


void OrbitSystem2D::Update(LV::Timestep dT)
{
    LV_PROFILE_FUNCTION();

    static const auto Tstart = std::chrono::steady_clock::now();


    for (uint32_t n = 1; n < m_InflNodes.size(); n++)
    {
        // Integrate true anomaly
        auto& op = m_InflNodes[n]->Parameters;

        // TODO : countdown timer - only update when enough time has passed for deltaTrueAnomaly to be similar
        // magnitude to op.True anomaly, to reduce precision error when summing them
        op.TrueAnomaly += m_Timescale * (float)dT * op.OSAMomentum.Float() / op.Position.SqrMagnitude(); // dTA / dt = h / r^2
        //LV::BigFloat deltaTrueAnomaly = op.OSAMomentum.Float() * (m_Timescale * (float)dT / op.Position.SqrMagnitude());
        //op.TrueAnomaly += deltaTrueAnomaly;
        if (op.TrueAnomaly > LV::PI2f)
        {
            op.TrueAnomaly -= LV::PI2f;


            // debug - integration accuracy
            auto Tfinal = std::chrono::steady_clock::now();
            float Ttotal = (Tfinal - Tstart).count() * 1e-9;
            float Tdur = (Tfinal - m_ActualPeriods[n]).count() * 1e-9;
            float period = op.Period.Float() / m_Timescale;
            LV_CORE_INFO("Orbiter {0}: theoretical period = {1}, recorded period = {2}, error = {3}ms", n, period, Tdur, (Tdur - period) * 1e3);
            m_Data[n] << Ttotal << "," << period << "," << Tdur << "," << ((Tdur - period) * 1e3) << std::endl;
            m_ActualPeriods[n] = Tfinal;
        }
        m_InflNodes[n]->NeedRecomputeState = true;
    }
}


void OrbitSystem2D::LoadLevel(const LV::BigFloat& hostMass, const LV::BigFloat& baseScaling)
{
    LV_PROFILE_FUNCTION();

    m_LevelHost.reset(new InfluencingNode);
    m_LevelHost->Id = 0;
    m_LevelHost->Influence.TotalScaling = baseScaling;
    m_LevelHost->Parameters.GravAsOrbiter = kGrav * hostMass;

    // Length dimension in G (the gravitational constant) is cubed - scaling must be cubed when computing scaled-GM
    m_LevelHost->Parameters.GravAsHost = m_LevelHost->Parameters.GravAsOrbiter / LV::BigFloat::Pow(baseScaling, 3);

    m_InflNodes.clear();
    m_InflNodes.push_back(m_LevelHost);
}


uint32_t OrbitSystem2D::CreateOrbiter(const LV::BigFloat& mass, LV::Vector2 scaledPosition, const LV::BigVector2& scaledVelocity)
{
    LV_PROFILE_FUNCTION();

    // Determine parent node (host of orbit)
    auto& p = FindLowestOverlappingInfluence(scaledPosition);

    return CreateInfluencingNode(p, mass, scaledPosition, scaledVelocity);
}


uint32_t OrbitSystem2D::CreateOrbiter(const LV::BigFloat& mass, LV::Vector2 scaledPosition, bool clockwise)
{
    LV_PROFILE_FUNCTION();

    // Determine parent node (host of orbit)
    auto& p = FindLowestOverlappingInfluence(scaledPosition);

    // Compute relative velocity of circular orbit
    LV::BigFloat vMag = LV::BigFloat::Sqrt(p->Parameters.GravAsHost / sqrtf(scaledPosition.SqrMagnitude()));
    LV::BigVector2 vDir = clockwise ?
        LV::BigVector2(scaledPosition.y, -scaledPosition.x).Normalized() :
        LV::BigVector2(-scaledPosition.y, scaledPosition.x).Normalized();

    return CreateInfluencingNode(p, mass, scaledPosition, vMag * vDir);
}


uint32_t OrbitSystem2D::CreateOrbiter(const LV::BigFloat& mass, const LV::BigVector2& position, const LV::BigVector2& velocity)
{
    LV_PROFILE_FUNCTION();

    // Determine parent node (host of orbit)
    LV::Vector2 scaledPosition = (position * m_LevelHost->Influence.TotalScaling).Vector2();
    auto& p = FindLowestOverlappingInfluence(scaledPosition);

    LV::BigVector2 scaledVelocity = (velocity - p->Parameters.Velocity) * p->Influence.TotalScaling;

    return CreateInfluencingNode(p, mass, scaledPosition, scaledVelocity);
}


uint32_t OrbitSystem2D::CreateOrbiter(const LV::BigFloat& mass, const LV::BigVector2& position, bool clockwise)
{
    LV_PROFILE_FUNCTION();

    // Determine parent node (host of orbit)
    LV::Vector2 scaledPosition = (position * m_LevelHost->Influence.TotalScaling).Vector2();

    return CreateOrbiter(mass, scaledPosition, clockwise);
}


uint32_t OrbitSystem2D::CreateInfluencingNode(const InflRef& parent, const LV::BigFloat& mass, const LV::Vector2& scaledPosition, const LV::BigVector2& scaledVelocity)
{
    LV_PROFILE_FUNCTION();

    InfluencingNode* newNode = new InfluencingNode;
    newNode->Parent = parent;

    // Compute gravitational properties of system
    auto& op = newNode->Parameters;
    op.GravAsOrbiter = parent->Parameters.GravAsHost; // mu = GM -> Assumes mass of orbiter is insignificant compared to host

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


    // debug //
    std::ostringstream fname;
    fname << LV_DIR << "/Profiling/OrbiterDebugData/orbiter-" << newNode->Id << ".txt";
    m_Filenames.emplace(newNode->Id, fname.str());
    m_Data.emplace(newNode->Id, std::ios_base::app | std::ios_base::trunc);
    m_Data[newNode->Id] << "Orbiter " << newNode->Id << " Debug Data" << std::endl;
    m_Data[newNode->Id] << "T,Theoretical Period,Recorded Period,Error (ms)" << std::endl;
    // debug //


    return newNode->Id;
}


void OrbitSystem2D::ComputeElementsFromState(OrbitParameters& op)
{
    LV_PROFILE_FUNCTION();
    // Some of these computations use optimisations which only apply
    // to orbits in the XY plane: assume the physics/maths used is
    // suitable only for 2D simulations!

    LV::BigFloat signedH = op.Position.x * op.Velocity.y - op.Position.y * op.Velocity.x; // z-component of Position cross Velocity
    int ccwF = signedH.GetCoefficient() < 0 ? -1.f : 1.f;
    op.OSAMomentum = LV::BigFloat::Abs(signedH);

    LV::Vector2 ur = op.Position.Normalized();
    LV::BigVector2 vCrossh = { op.Velocity.y * signedH, -op.Velocity.x * signedH };
    LV::Vector2 eVec = (vCrossh / op.GravAsOrbiter).Vector2() - ur;
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
        op.BasisX = ur;
    }
    LV::Vector2 cwHorz = { -op.Position.y, op.Position.x };
    op.BasisY = ccwF * LV::Vector2(- op.BasisX.y, op.BasisX.x);

    op.TrueAnomaly = acosf(op.BasisX.Dot(ur));
    if (((op.Velocity.x * ur.x) + (op.Velocity.y * ur.y)).GetCoefficient() < 0) // disambiguate quadrant - is Velocity on the inside of the tangent vector?
    {
        op.TrueAnomaly = LV::PI2f - op.TrueAnomaly;
    }

    op.RightAscensionPeriapsis = acosf(op.BasisX.x);
    if (op.BasisX.y < 0) // quadrant disambiguation - is periapsis above or below the reference frame's X-axis?
    {
        op.RightAscensionPeriapsis = LV::PI2f - op.RightAscensionPeriapsis;
    }

    LV::BigFloat h2 = LV::BigFloat::Pow(op.OSAMomentum, 2);
    LV::BigFloat bigh2mu = h2 / op.GravAsOrbiter;
    op.h2mu = bigh2mu.Float();
    op.muh = op.GravAsOrbiter / op.OSAMomentum;

    float oneMinusE2 = (1.f - e2);
    op.SemiMajorAxis = op.h2mu / oneMinusE2;
    op.SemiMinorAxis = op.SemiMajorAxis * sqrtf(oneMinusE2);
    op.Centre = -op.SemiMajorAxis * op.Eccentricity * op.BasisX;

    op.Period = LV::PI2f * op.SemiMajorAxis * op.SemiMinorAxis / op.OSAMomentum;
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


void OrbitSystem2D::ComputeInfluence(InfluencingNode* influencingNode, const InflRef& parent, const LV::BigFloat& mass)
{
    LV_PROFILE_FUNCTION();

    auto& op = influencingNode->Parameters;
    auto& infl = influencingNode->Influence;

    LV::BigFloat parentScaledGrav = kGrav * mass * LV::BigFloat::Pow(parent->Influence.TotalScaling, 3);
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
    infl.Radius = op.SemiMajorAxis * pow((parentScaledGrav / op.GravAsOrbiter).Float(), 0.4f); // roi = a(m/M)^(5/2)
    infl.TotalScaling = parent->Influence.TotalScaling / LV::BigFloat(infl.Radius);
    op.GravAsHost = kGrav * mass * LV::BigFloat::Pow(infl.TotalScaling, 3); // G's length dimension is cubed - scaling must be cubed: scaled-GM = GM / scale^3
}


OrbitSystem2D::InflRef& OrbitSystem2D::FindLowestOverlappingInfluence(LV::Vector2& scaledPosition)
{
    LV_PROFILE_FUNCTION();

    uint32_t parentId = 0, inflNodeId;
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


OrbitSystem2D::InflRef& OrbitSystem2D::FindOverlappingChildInfluence(InflRef& parent, const LV::Vector2& scaledPosition)
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

    return m_InflNodes[host]->Influence.TotalScaling.Float();
}


float OrbitSystem2D::GetHostScaling(const uint32_t orbiter)
{
    LV_CORE_ASSERT(orbiter < m_InflNodes.size() && orbiter > 0, "Invalid orbiter ID!");

    return m_InflNodes[orbiter]->Parent->Influence.TotalScaling.Float();
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
        op.TrueAnomaly += LV::PI2f;
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
