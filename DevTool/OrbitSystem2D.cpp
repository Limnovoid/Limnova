#include "OrbitSystem2D.h"

#include <chrono>


namespace LV = Limnova;

static const LV::BigFloat kGrav = { 6.6743f, -11 };
static constexpr float kOptimalDeltaTAnom = 0.001f;


OrbitSystem2D OrbitSystem2D::s_OrbitSystem2D;

void OrbitSystem2D::Init()
{
    LV_PROFILE_FUNCTION();

    s_OrbitSystem2D.m_LevelHost.reset();
    s_OrbitSystem2D.m_InflNodes.clear();


    // debug - orbiter integration accuracy
    s_OrbitSystem2D.m_DebugData.clear();
}


OrbitSystem2D& OrbitSystem2D::Get()
{
    return s_OrbitSystem2D;
}


void OrbitSystem2D::Shutdown()
{
    LV_PROFILE_FUNCTION();

    if (s_OrbitSystem2D.m_Testing)
    {
        s_OrbitSystem2D.RecordData();
    }
}


void OrbitSystem2D::Update(LV::Timestep dT)
{
    LV_PROFILE_FUNCTION();


    // debug - integration accuracy
    static auto TStart = std::chrono::steady_clock::now(); // Not safe with window interrupts - should use game time
    static auto TPreviousFrame = TStart;
    static std::chrono::steady_clock::time_point TThisFrame;
    TThisFrame = std::chrono::steady_clock::now();
    // debug - integration accuracy


    for (uint32_t n = 1; n < m_InflNodes.size(); n++)
    {
        // Integrate true anomaly
        auto& orbiter = m_InflNodes[n];
        auto& op = orbiter->Parameters;


        // debug - integration accuracy
        float oldTAnomaly = op.TrueAnomaly;
        // debug - integration accuracy


        // Disconnected update loop:
        // To handle cases where the simulated orbit period is much longer than a frame (such that per-frame changes in
        // true anomaly would be very small in comparison, potentially introducing significant finite-precision error),
        // orbit position is only updated after a certain amount of time (governed by op.UpdateTimer).
        // This longer timestep (simDeltaTime) is computed to ensure that the change in true anomaly (simDeltaTAnomaly)
        // is equal to or greater than kOptimalDeltaTAnom.
        float gameDeltaTime = m_Timescale * (float)dT;
        while (op.UpdateTimer < 0)
        {
            // dTAnom / dT = h / r^2 --> dT_optimal = dTAnom_optimal * r^2 / h
            float simDeltaTAnomaly = kOptimalDeltaTAnom;
            float simDeltaTime = (kOptimalDeltaTAnom * op.Position.SqrMagnitude() / op.OSAMomentum).Float();

            // TEMPORARY - never allows multiple updates per frame
            // TODO - allow multiple updates to maintain integration accuracy for faster orbiters (and while speeding-up time)
            if (simDeltaTime < gameDeltaTime)
            {
                // dTAnom = dT * h / r^2
                simDeltaTime = gameDeltaTime;
                simDeltaTAnomaly = gameDeltaTime * op.OSAMomentum.Float() / op.Position.SqrMagnitude();
            }

            op.TrueAnomaly += simDeltaTAnomaly;
            op.UpdateTimer += simDeltaTime;

            // Check influence events:
            // If distance between this orbiter and another orbiter of the same host is less than that orbiter's
            // ROI, this orbiter is now orbiting the other orbiter
            // TODO ?? different scheme for checking overlaps if multiple updates can occur per frame:
            // - checking overlaps between influencing orbiters which may have multiple updates per frame would require
            // - updating all orbiters once, checking overlaps, updating and checking again, etc. until all orbiters are
            // - fully updated, every frame.
            
            // If true anomaly is greater than true anomaly of escape, orbiter has escaped its host's influence
            if (orbiter->Parent != m_LevelHost
                && op.TrueAnomalyEscape > 0
                && op.TrueAnomaly > op.TrueAnomalyEscape)
            {
                // Convert position and velocity (which are stored relative and scaled to host's orbit space) to new host's orbit space
                auto& host = orbiter->Parent;
                op.Position = host->Parameters.Position + (op.Position * host->Influence.Radius);
                op.Velocity = host->Parameters.Velocity + (op.Velocity * host->Influence.Radius);

                // Update orbit tree:
                // Remove node from old parent's children
                auto childrenOrbiterIt = std::find(host->InfluencingChildren.begin(), host->InfluencingChildren.end(), orbiter);
                size_t childrenLastIdx = host->InfluencingChildren.size() - 1;
                childrenOrbiterIt->swap(host->InfluencingChildren[childrenLastIdx]);
                host->InfluencingChildren.resize(childrenLastIdx);
                // Add node to new parent's children
                host->Parent->InfluencingChildren.push_back(orbiter);

                orbiter->Parent = host->Parent;

                ComputeElementsFromState(op);
                ComputeInfluence(orbiter.get());

                m_OrbiterChangedHostCallback(n);
            }

            ComputeStateVector(op);
        }
        orbiter->NeedRecomputeState = false;
        op.UpdateTimer -= gameDeltaTime;

        //float deltaTA = m_Timescale * (float)dT * (op.OSAMomentum / op.Position.SqrMagnitude()).Float();
        //op.TrueAnomaly += deltaTA;

        if (op.TrueAnomaly > LV::PI2f)
        {
            op.TrueAnomaly -= LV::PI2f;


            // debug - integration accuracy
            if (m_Testing)
            {
                auto TDelta = TThisFrame - TPreviousFrame;
                auto TActualPeriapsePass = TPreviousFrame + (TDelta * (LV::PI2f - oldTAnomaly) / (oldTAnomaly - op.TrueAnomaly));
                auto& data = m_DebugData[n];
                if (data.NumPeriapsePasses == 0)
                {
                    data.TFirstPeriapsePass = TActualPeriapsePass;
                }
                else
                {
                    // Collect data
                    float actualPeriapsePass = 1e-9 * (TActualPeriapsePass - data.TFirstPeriapsePass).count();
                    float predictedPeriapsePass = data.NumPeriapsePasses * (op.Period / m_Timescale).Float(); // Assumes constant timescale
                    float errorPeriapsePass = 1e3 * (predictedPeriapsePass - actualPeriapsePass);
                    LV_CORE_INFO("Orbiter {0}: predicted periapse passage = {1} s, actual passage = {2} s, error = {3} ms", n, predictedPeriapsePass, actualPeriapsePass, errorPeriapsePass);
                    float simulationTime = 1e-9 * (TThisFrame - TStart).count();
                    data.Table->AddRow(simulationTime,
                        data.NumPeriapsePasses,
                        predictedPeriapsePass,
                        actualPeriapsePass,
                        errorPeriapsePass
                    );
                }
                data.NumPeriapsePasses++;

                // Record all data after orbiter 1 has completed 5 orbits.
                if (n == 1 && data.NumPeriapsePasses > 5)
                {
                    RecordData();
                    m_Testing = false;
                    LV_CORE_ASSERT(false, "Orbiter integration test run complete");
                }
            }
            // debug - integration accuracy
        }
    }


    // debug - integration accuracy
    TPreviousFrame = TThisFrame;
}


void OrbitSystem2D::LoadLevel(const LV::BigFloat& hostMass, const LV::BigFloat& baseScaling)
{
    LV_PROFILE_FUNCTION();

    m_LevelHost.reset(new InfluencingNode);
    m_LevelHost->Id = 0;
    m_LevelHost->Mass = hostMass;
    m_LevelHost->Parameters.GravAsOrbiter = kGrav * hostMass;
    m_LevelHost->Influence.TotalScaling = baseScaling;

    // Length dimension in G (the gravitational constant) is cubed - scaling must be cubed when computing scaled-GM
    m_LevelHost->Parameters.GravAsHost = m_LevelHost->Parameters.GravAsOrbiter / LV::BigFloat::Pow(baseScaling, 3);

    m_InflNodes.clear();
    m_InflNodes.push_back(m_LevelHost);
}


uint32_t OrbitSystem2D::CreateOrbiter(const LV::BigFloat& mass, LV::Vector2 scaledPosition, LV::BigVector2& scaledVelocity)
{
    LV_PROFILE_FUNCTION();

    // Determine parent node (host of orbit)
    auto& p = FindLowestOverlappingInfluence(scaledPosition, scaledVelocity);

    return CreateInfluencingNode(p, mass, scaledPosition, scaledVelocity);
}


uint32_t OrbitSystem2D::CreateOrbiter(const LV::BigFloat& mass, LV::Vector2 scaledPosition, bool clockwise)
{
    LV_PROFILE_FUNCTION();

    // Determine parent node (host of orbit)
    LV::BigVector2 scaledVelocity;
    auto& p = FindLowestOverlappingInfluence(scaledPosition, scaledVelocity);

    // Compute relative velocity of circular orbit
    LV::BigFloat vMag = LV::BigFloat::Sqrt(p->Parameters.GravAsHost / sqrtf(scaledPosition.SqrMagnitude()));
    LV::BigVector2 vDir = clockwise ?
        LV::BigVector2(scaledPosition.y, -scaledPosition.x).Normalized() :
        LV::BigVector2(-scaledPosition.y, scaledPosition.x).Normalized();
    scaledVelocity = vMag * vDir;

    return CreateInfluencingNode(p, mass, scaledPosition, scaledVelocity);
}


uint32_t OrbitSystem2D::CreateOrbiter(const LV::BigFloat& mass, const LV::BigVector2& position, const LV::BigVector2& velocity)
{
    LV_PROFILE_FUNCTION();

    // Determine parent node (host of orbit)
    LV::Vector2 scaledPosition = (position * m_LevelHost->Influence.TotalScaling).Vector2();
    LV::BigVector2 scaledVelocity = velocity * m_LevelHost->Influence.TotalScaling;
    auto& p = FindLowestOverlappingInfluence(scaledPosition, scaledVelocity);

    return CreateInfluencingNode(p, mass, scaledPosition, scaledVelocity);
}


uint32_t OrbitSystem2D::CreateOrbiter(const LV::BigFloat& mass, const LV::BigVector2& position, bool clockwise)
{
    LV_PROFILE_FUNCTION();

    LV::Vector2 scaledPosition = (position * m_LevelHost->Influence.TotalScaling).Vector2();

    return CreateOrbiter(mass, scaledPosition, clockwise);
}


uint32_t OrbitSystem2D::CreateInfluencingNode(const InflRef& parent, const LV::BigFloat& mass, const LV::Vector2& scaledPosition, const LV::BigVector2& scaledVelocity)
{
    LV_PROFILE_FUNCTION();

    InfluencingNode* newNode = new InfluencingNode;
    newNode->Parent = parent;

    // Compute gravitational properties of system
    newNode->Mass = mass;
    auto& op = newNode->Parameters;
    op.GravAsOrbiter = parent->Parameters.GravAsHost; // mu = GM -> Assumes mass of orbiter is insignificant compared to host

    // Compute orbital elements
    op.Position = scaledPosition;
    op.Velocity = scaledVelocity;
    newNode->NeedRecomputeState = false;
    ComputeElementsFromState(op);

    // Compute this orbiter's influence
    ComputeInfluence(newNode);

    // Add node to tree
    newNode->Id = m_InflNodes.size();
    newNode->Parent->InfluencingChildren.push_back(m_InflNodes.emplace_back(newNode));


    // debug //
    m_DebugData.emplace(newNode->Id, std::make_shared<Limnova::CsvTable<float, uint32_t, float, float, float>>());
    m_DebugData[newNode->Id].Table->Init(
        "OrbiterDebugData/orbiter" + std::to_string(newNode->Id) + ".txt",
        "Orbiter Debug Data: Orbiter " + std::to_string(newNode->Id),
        { "T (s)", "Num.Passes", "Predicted Pass Time(s)", "Actual Pass Time(s)", "Error(ms)" }, false
    );
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
    op.CcwF = signedH.GetCoefficient() < 0 ? -1.f : 1.f;
    op.OSAMomentum = LV::BigFloat::Abs(signedH);

    LV::Vector2 ur = op.Position.Normalized();
    LV::BigVector2 vCrossh = { op.Velocity.y * signedH, -op.Velocity.x * signedH };
    LV::Vector2 eVec = (vCrossh / op.GravAsOrbiter).Vector2() - ur;
    float e2 = eVec.SqrMagnitude();
    LV_CORE_ASSERT(e2 < 1, "Parapolic and hyperbolic trajectories are not currently supported!");
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
    op.BasisY = op.CcwF * LV::Vector2(- op.BasisX.y, op.BasisX.x);

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

    op.OParameter = (LV::BigFloat::Pow(op.OSAMomentum, 2) / op.GravAsOrbiter).Float();
    op.muh = op.GravAsOrbiter / op.OSAMomentum;

    float oneMinusE2 = (1.f - e2);
    op.SemiMajorAxis = op.OParameter / oneMinusE2;
    op.SemiMinorAxis = op.SemiMajorAxis * sqrtf(oneMinusE2);
    op.Centre = -op.SemiMajorAxis * op.Eccentricity * op.BasisX;

    op.Period = LV::PI2f * op.SemiMajorAxis * op.SemiMinorAxis / op.OSAMomentum;

    // If distance to apoapsis is greater than 1, the orbiter will leave the host's influence:
    // r_a = h^2 / mu(1 - e)
    if (op.OParameter / (1.f - op.Eccentricity) > 1)
    {
        // Orbiter leaves host's influence at the point that its orbital distance is 1:
        // cos(TAnomaly) = (h^2 / (mu * r) - 1) / e
        op.TrueAnomalyEscape = acosf((op.OParameter - 1) / op.Eccentricity);
    }
    else
    {
        op.TrueAnomalyEscape = -1.f;
    }
}


void OrbitSystem2D::ComputeStateVector(OrbitParameters& params)
{
    LV_PROFILE_FUNCTION();

    float sinT = sin(params.TrueAnomaly);
    float cosT = cos(params.TrueAnomaly);
    params.Position = params.OParameter
        * (params.BasisX * cosT + params.BasisY * sinT)
        / (1.f + params.Eccentricity * cosT);

    params.Velocity = params.muh
        * (params.BasisY * (params.Eccentricity + cosT) - params.BasisX * sinT);
}


void OrbitSystem2D::ComputeInfluence(InfluencingNode* influencingNode)
{
    LV_PROFILE_FUNCTION();
    auto& parentInfl = influencingNode->Parent->Influence;
    auto& op = influencingNode->Parameters;
    auto& infl = influencingNode->Influence;

#ifdef LV_DEBUG
    auto& hp = influencingNode->Parent->Parameters;
    if (influencingNode->Mass.GetExponent() > influencingNode->Parent->Mass.GetExponent()
        - (int)cbrtf((float)influencingNode->Parent->Mass.GetExponent()) - 1)
    {
        LV_CORE_ERROR("Orbiter mass ({0}) is too high for lowest overlapping circle of influence (orbiter {1} with mass {2})!", influencingNode->Mass, influencingNode->Parent->Id, influencingNode->Parent->Mass);
        delete influencingNode;
        LV_CORE_ASSERT(false, "");
    }
#endif
    infl.Radius = op.SemiMajorAxis * LV::BigFloat::PowF(influencingNode->Mass / influencingNode->Parent->Mass, 0.4f).Float(); // roi = a(m/M)^(5/2)
    infl.TotalScaling = parentInfl.TotalScaling / LV::BigFloat(infl.Radius);
    op.GravAsHost = kGrav * influencingNode->Mass * LV::BigFloat::Pow(infl.TotalScaling, 3); // G's length dimension is cubed - scaling must be cubed: scaled-GM = GM / scale^3
}


OrbitSystem2D::InflRef& OrbitSystem2D::FindLowestOverlappingInfluence(LV::Vector2& scaledPosition, Limnova::BigVector2& scaledVelocity)
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
        scaledVelocity = (scaledVelocity - inflNode->Parameters.Velocity) / inflNode->Influence.Radius;
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


void OrbitSystem2D::IncreaseOrbitLevel(InflRef& orbiter)
{
    LV_PROFILE_FUNCTION();
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


uint32_t OrbitSystem2D::GetHost(const uint32_t orbiter)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(orbiter < m_InflNodes.size() && orbiter > 0, "Invalid orbiter ID!");

    return m_InflNodes[orbiter]->Parent->Id;
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


void OrbitSystem2D::RecordData()
{
    for (uint32_t n = 1; n < m_InflNodes.size(); n++)
    {
        m_DebugData[n].Table->Write();
    }
}
