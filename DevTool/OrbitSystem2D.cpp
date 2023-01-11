#include "OrbitSystem2D.h"

#include <chrono>


namespace LV = Limnova;

static const LV::BigFloat kGrav = { 6.6743f, -11 };
static constexpr float kOptimalDeltaTAnom = 0.001f;
static constexpr float kEscapeDistance = 1.01f;


OrbitSystem2D OrbitSystem2D::s_OrbitSystem2D;

void OrbitSystem2D::Init()
{
    LV_PROFILE_FUNCTION();

    s_OrbitSystem2D.m_LevelHost.reset();
    s_OrbitSystem2D.m_DynamicNodes.clear();


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


    for (uint32_t n = 1; n < m_DynamicNodes.size(); n++)
    {
        // Integrate true anomaly
        auto& orbiter = m_DynamicNodes[n];
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
            
            // If true anomaly is greater than true anomaly of escape (and less than pi), orbiter has escaped its host's influence;
            // however, if true anomaly is greater than pi, orbiter is still inside the influence and is approaching periapsis.
            if (orbiter->Parent != m_LevelHost
                && op.TrueAnomaly > op.TrueAnomalyEscape
                && op.TrueAnomaly < LV::PIf)
            {
                // Convert position and velocity (which are stored relative and scaled to host's orbit space) to new host's orbit space
                auto& host = orbiter->Parent;
                op.HostEscaped = host->Id;
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

                orbiter->ComputeElementsFromState();
                orbiter->ComputeInfluence();

                m_OrbiterChangedHostCallback(n);
            }

            orbiter->ComputeStateVector();

            // Test if this orbiter is overlapped by the circle of influence of any orbiters of the same host
            for (auto& other : orbiter->Parent->InfluencingChildren)
            {
                if (orbiter == other) continue; // Skip self

                float separation = sqrt((other->Parameters.Position - orbiter->Parameters.Position).SqrMagnitude());

                if (separation < other->Influence.Radius
                    && orbiter->Parameters.HostEscaped != other->Id)
                {
                    // Overlap
                    LV_CORE_INFO("Overlap: orbiter {0} -> influence {1}!", orbiter->Id, other->Id);
                }
                else if (orbiter->Parameters.HostEscaped == other->Id
                    && separation > other->Influence.Radius)
                {
                    // Previous host is for sure escaped
                    LV_CORE_INFO("Escape: orbiter {0} <- influence {1}!", orbiter->Id, other->Id);
                    orbiter->Parameters.HostEscaped = s_OrbitSystem2D.m_LevelHost->Id;
                }
            }
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

    m_DynamicNodes.clear();
    m_DynamicNodes.push_back(m_LevelHost);
}


uint32_t OrbitSystem2D::CreateOrbiterES(const bool dynamic, const LV::BigFloat& mass, const uint32_t initialHostId, LV::Vector2 scaledPosition, LV::BigVector2 scaledVelocity)
{
    LV_PROFILE_FUNCTION();

    // Determine parent node (host of orbit)
    auto& p = FindLowestOverlappingInfluence(scaledPosition, scaledVelocity, initialHostId);

    return CreateInfluencingNode(p, mass, scaledPosition, scaledVelocity);
}


uint32_t OrbitSystem2D::CreateOrbiterCS(const bool dynamic, const LV::BigFloat& mass, const uint32_t initialHostId, LV::Vector2 scaledPosition, const bool clockwise)
{
    LV_PROFILE_FUNCTION();

    // Determine parent node (host of orbit)
    LV::BigVector2 scaledVelocity;
    auto& p = FindLowestOverlappingInfluence(scaledPosition, scaledVelocity, initialHostId);

    // Compute relative velocity of circular orbit
    LV::BigFloat vMag = LV::BigFloat::Sqrt(p->Parameters.GravAsHost / sqrtf(scaledPosition.SqrMagnitude()));
    LV::BigVector2 vDir = clockwise ?
        LV::BigVector2(scaledPosition.y, -scaledPosition.x).Normalized() :
        LV::BigVector2(-scaledPosition.y, scaledPosition.x).Normalized();
    scaledVelocity = vMag * vDir;

    return CreateInfluencingNode(p, mass, scaledPosition, scaledVelocity);
}


uint32_t OrbitSystem2D::CreateOrbiterEU(const bool dynamic, const LV::BigFloat& mass, const LV::BigVector2& position, const LV::BigVector2& velocity)
{
    LV_PROFILE_FUNCTION();

    // Determine parent node (host of orbit)
    LV::Vector2 scaledPosition = (position * m_LevelHost->Influence.TotalScaling).Vector2();
    LV::BigVector2 scaledVelocity = velocity * m_LevelHost->Influence.TotalScaling;
    auto& p = FindLowestOverlappingInfluence(scaledPosition, scaledVelocity);

    return CreateInfluencingNode(p, mass, scaledPosition, scaledVelocity);
}


uint32_t OrbitSystem2D::CreateOrbiterCU(const bool dynamic, const LV::BigFloat& mass, const LV::BigVector2& position, const bool clockwise)
{
    LV_PROFILE_FUNCTION();

    LV::Vector2 scaledPosition = (position * m_LevelHost->Influence.TotalScaling).Vector2();

    return CreateOrbiterCS(dynamic, mass, 0, scaledPosition, clockwise);
}


uint32_t OrbitSystem2D::CreateInfluencingNode(const InflRef& parent, const LV::BigFloat& mass, const LV::Vector2& scaledPosition, const LV::BigVector2& scaledVelocity)
{
    LV_PROFILE_FUNCTION();

    InfluencingNode* newNode = new InfluencingNode;
    newNode->Id = m_DynamicNodes.size();
    newNode->Parent = parent;

    // Compute gravitational properties of system
    newNode->Mass = mass;
    auto& op = newNode->Parameters;
    op.GravAsOrbiter = parent->Parameters.GravAsHost; // mu = GM -> Assumes mass of orbiter is insignificant compared to host

    // Compute orbital elements
    op.Position = scaledPosition;
    op.Velocity = scaledVelocity;
    newNode->NeedRecomputeState = false;
    newNode->ComputeElementsFromState();

    // Compute this orbiter's influence
    newNode->ComputeInfluence();

    // Add to tree
    newNode->Parent->InfluencingChildren.push_back(m_DynamicNodes.emplace_back(newNode));

    // debug //
    m_DebugData.emplace(newNode->Id, std::make_shared<Limnova::CsvTable<float, uint32_t, float, float, float>>());
    m_DebugData[newNode->Id].Table->Init(
        "Orbiter Debug Data: Orbiter " + std::to_string(newNode->Id),
        "OrbiterDebugData/orbiter" + std::to_string(newNode->Id) + ".txt",
        { "T (s)", "Num.Passes", "Predicted Pass Time(s)", "Actual Pass Time(s)", "Error(ms)" }, false
    );
    // debug //


    return newNode->Id;
}


void OrbitSystem2D::OrbitTreeNode::ComputeElementsFromState()
{
    LV_PROFILE_FUNCTION();

    auto& op = this->Parameters;

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

    // Predicting orbit events:
    // If distance to apoapsis is greater than escape distance, the orbiter will leave the host's influence:
    // r_a = h^2 / mu(1 - e)
    if (this->Parent != s_OrbitSystem2D.m_LevelHost
        && op.OParameter / (1.f - op.Eccentricity) > kEscapeDistance)
    {
        // Orbiter leaves host's influence at the point that its orbital distance is equal to the escape distance (r_esc):
        // cos(TAnomaly) = (h^2 / (mu * r_esc) - 1) / e
        op.TrueAnomalyEscape = acosf((op.OParameter / kEscapeDistance - 1.f) / op.Eccentricity);
        LV_CORE_INFO("Orbiter {0} will escape {1} at true anomaly {2} (current true anomaly {3})", this->Id, this->Parent->Id, op.TrueAnomalyEscape, op.TrueAnomaly);
    }
    else
    {
        op.TrueAnomalyEscape = 2.f * LV::PI2f;
    }
    // Orbit intersects:
    // TODO
}


void OrbitSystem2D::OrbitTreeNode::ComputeStateVector()
{
    LV_PROFILE_FUNCTION();

    auto& op = this->Parameters;

    float sinT = sin(op.TrueAnomaly);
    float cosT = cos(op.TrueAnomaly);
    op.Position = op.OParameter
        * (op.BasisX * cosT + op.BasisY * sinT)
        / (1.f + op.Eccentricity * cosT);

    op.Velocity = op.muh * (op.BasisY * (op.Eccentricity + cosT) - op.BasisX * sinT);
}


void OrbitSystem2D::InfluencingNode::ComputeInfluence()
{
    LV_PROFILE_FUNCTION();
    auto& parentInfl = this->Parent->Influence;
    auto& op = this->Parameters;
    auto& infl = this->Influence;

#ifdef LV_DEBUG
    auto& hp = this->Parent->Parameters;
    if (this->Mass.GetExponent() > this->Parent->Mass.GetExponent()
        - (int)cbrtf((float)this->Parent->Mass.GetExponent()) - 1)
    {
        LV_CORE_ERROR("Orbiter mass ({0}) is too high for lowest overlapping circle of influence (orbiter {1} with mass {2})!", this->Mass, this->Parent->Id, this->Parent->Mass);
        LV_CORE_ASSERT(false, "");
    }
#endif
    infl.Radius = op.SemiMajorAxis * LV::BigFloat::PowF(this->Mass / this->Parent->Mass, 0.4f).Float(); // roi = a(m/M)^(5/2)
    infl.TotalScaling = parentInfl.TotalScaling / LV::BigFloat(infl.Radius);
    op.GravAsHost = kGrav * this->Mass * LV::BigFloat::Pow(infl.TotalScaling, 3); // G's length dimension is cubed - scaling must be cubed: scaled-GM = GM / scale^3
}


OrbitSystem2D::InflRef& OrbitSystem2D::FindLowestOverlappingInfluence(LV::Vector2& scaledPosition, Limnova::BigVector2& scaledVelocity, const uint32_t initialHostId)
{
    LV_PROFILE_FUNCTION();

    uint32_t parentId = initialHostId;
    for (uint32_t i = 0; i < m_DynamicNodes.size(); i++)
    {
        auto& inflNode = FindOverlappingChildInfluence(m_DynamicNodes[parentId], scaledPosition);
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


const OrbitSystem2D::OrbitTreeNode& OrbitSystem2D::GetOrbiter(const uint32_t id)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(id < m_DynamicNodes.size() && id >= 0, "Invalid orbiter ID!");

    auto& node = m_DynamicNodes[id];
    if (node->NeedRecomputeState)
    {
        node->ComputeStateVector();
        node->NeedRecomputeState = false;
    }
    return *node;
}


const OrbitSystem2D::InfluencingNode& OrbitSystem2D::GetHost(const uint32_t id)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(id < m_DynamicNodes.size() && id >= 0, "Invalid orbiter ID!");

    auto& node = m_DynamicNodes[id];
    if (node->NeedRecomputeState)
    {
        node->ComputeStateVector();
        node->NeedRecomputeState = false;
    }
    return *node;
}


const OrbitSystem2D::OrbitParameters& OrbitSystem2D::GetParameters(const uint32_t orbiter)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(orbiter < m_DynamicNodes.size() && orbiter >= 0, "Invalid orbiter ID!");

    auto& node = m_DynamicNodes[orbiter];
    if (node->NeedRecomputeState)
    {
        node->ComputeStateVector();
        node->NeedRecomputeState = false;
    }
    return m_DynamicNodes[orbiter]->Parameters;
}


float OrbitSystem2D::GetRadiusOfInfluence(const uint32_t orbiter)
{
    LV_CORE_ASSERT(orbiter < m_DynamicNodes.size() && orbiter >= 0, "Invalid orbiter ID!");

    return m_DynamicNodes[orbiter]->Influence.Radius;
}


float OrbitSystem2D::GetScaling(const uint32_t host)
{
    LV_CORE_ASSERT(host < m_DynamicNodes.size() && host >= 0, "Invalid orbiter ID!");

    return m_DynamicNodes[host]->Influence.TotalScaling.Float();
}


float OrbitSystem2D::GetHostScaling(const uint32_t orbiter)
{
    LV_CORE_ASSERT(orbiter < m_DynamicNodes.size() && orbiter > 0, "Invalid orbiter ID!");

    return m_DynamicNodes[orbiter]->Parent->Influence.TotalScaling.Float();
}


void OrbitSystem2D::GetChildren(const uint32_t host, std::vector<uint32_t>& ids)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(host < m_DynamicNodes.size() && host >= 0, "Invalid orbiter ID!");

    for (auto& child : m_DynamicNodes[host]->InfluencingChildren)
    {
        ids.push_back(child->Id);
    }
}


uint32_t OrbitSystem2D::GetOrbiterHost(const uint32_t orbiter)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(orbiter < m_DynamicNodes.size() && orbiter > 0, "Invalid orbiter ID!");

    return m_DynamicNodes[orbiter]->Parent->Id;
}


void OrbitSystem2D::SetOrbiterRightAscension(const uint32_t orbiter, const float rightAscension)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(orbiter > 0, "Cannot set right ascension of level host!");

    auto& op = m_DynamicNodes[orbiter]->Parameters;

    op.TrueAnomaly = op.CcwF > 0 ? rightAscension - op.RightAscensionPeriapsis : op.RightAscensionPeriapsis - rightAscension;
    if (op.TrueAnomaly < 0)
    {
        op.TrueAnomaly += LV::PI2f;
    }

    m_DynamicNodes[orbiter]->NeedRecomputeState = true;
}


void OrbitSystem2D::GetAllHosts(std::vector<uint32_t>& ids)
{
    LV_PROFILE_FUNCTION();

    for (auto& infl : m_DynamicNodes)
    {
        ids.push_back(infl->Id);
    }
}


void OrbitSystem2D::RecordData()
{
    for (uint32_t n = 1; n < m_DynamicNodes.size(); n++)
    {
        m_DebugData[n].Table->Write();
    }
}
