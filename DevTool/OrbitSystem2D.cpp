#include "OrbitSystem2D.h"

#include <chrono>


namespace LV = Limnova;

static const LV::BigFloat kGrav = { 6.6743f, -11 };
static constexpr float kMinimumDeltaTAnom = 1e-4f;
static constexpr float kMinimumDeltaT = 1.f / (60.f * 20.f); // maximum 20 updates per node per frame at 60 frames per second
static constexpr float kEscapeDistance = 1.01f;
static constexpr float kNumTrajectoryEscapePointsScene = 16; // Number of points used to draw the path of a trajectory, from periapsis to escape


OrbitSystem2D::OrbitSystem2D()
{
    m_MinimumDeltaT = m_Timescale * kMinimumDeltaT;
}


OrbitSystem2D::~OrbitSystem2D()
{
}


OrbitSystem2D OrbitSystem2D::s_OrbitSystem2D;

void OrbitSystem2D::Init()
{
    LV_PROFILE_FUNCTION();

    s_OrbitSystem2D.m_LevelHost.reset();
    s_OrbitSystem2D.m_AllNodes.clear();


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


void OrbitSystem2D::Update(Limnova::Timestep dT)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(m_UpdateFirst.get() != nullptr, "Update queue head pointer is null!");


    std::ostringstream debugoss; // debug
    debugoss << "Node update counts:\n"; // debug


    // Update all orbit nodes:
    // Nodes are queued in ascending order of their individual times until next update (stored in OrbitParameters::UpdateTimer),
    // which is measured from the start of the current frame - when a node is updated, its UpdateTimer increases by the size of
    // its individual timestep. The queue is iterated through in order until all UpdateTimers are greater than the gameDeltaTime.
    // This allows nodes to be updated with different time steps, zero or more times per frame (for each node), while still
    // updating them all chronologically for more accurate collision tracking.
    float gameDeltaTime = m_Timescale * (float)dT;
    while (m_UpdateFirst->Parameters.UpdateTimer < gameDeltaTime)
    {
        m_UpdateCounts[m_UpdateFirst->Id]++; // debug


        auto& op = m_UpdateFirst->Parameters;

        float r2 = op.Position.SqrMagnitude();
        float nodeDeltaTime = ((kMinimumDeltaTAnom * r2) / op.OSAMomentum).Float();
        float nodeDeltaTAnomaly = kMinimumDeltaTAnom;

        if (nodeDeltaTime < m_MinimumDeltaT)
        {
            // Limit number of updates per node per frame - see kMinimumDeltaT
            nodeDeltaTime = m_MinimumDeltaT;
            nodeDeltaTAnomaly = (m_MinimumDeltaT * op.OSAMomentum / r2).Float();
        }

        op.UpdateTimer += nodeDeltaTime;
        op.TrueAnomaly += nodeDeltaTAnomaly;
        if (op.TrueAnomaly > LV::PI2f)
        {
            op.TrueAnomaly -= LV::PI2f;
        }

        m_UpdateFirst->ComputeStateVector();

        // Handle orbit events:
        if (m_UpdateFirst->Dynamic)
        {
            HandleOrbiterEscapingHost(m_UpdateFirst);
            HandleOrbiterOverlappingInfluence(m_UpdateFirst);
        }

        UpdateQueueSortFirst();
    }
    // Per-frame orbit node updates complete: subtract gameDeltaTime from all UpdateTimers.
    OrbitTreeNode* node = m_UpdateFirst.get();
    while (node != nullptr)
    {
        node->Parameters.UpdateTimer -= gameDeltaTime;

        // debug
        uint32_t num = m_UpdateCounts[node->Id];
        debugoss << "- " << node->Id << ": " << num << '\n';
        m_UpdateCounts[node->Id] = 0;
        // debug

        node = node->m_UpdateNext.get();
    }

    std::cout << debugoss.str() << std::endl;
}


void OrbitSystem2D::UpdateQueueSortFirst()
{
    LV_PROFILE_FUNCTION();

    NodeRef rFirst = m_UpdateFirst;
    LV_CORE_ASSERT(rFirst.get() != nullptr, "Update queue head pointer is null!"); // Any level will always have at least the player ship in the update queue ?

    OrbitTreeNode* pOther = rFirst->m_UpdateNext.get();
    if (pOther == nullptr || rFirst->Parameters.UpdateTimer < pOther->Parameters.UpdateTimer)
    {
        return;
    }

    m_UpdateFirst = m_UpdateFirst->m_UpdateNext;

    while (pOther->m_UpdateNext.get() != nullptr)
    {
        if (rFirst->Parameters.UpdateTimer < pOther->m_UpdateNext->Parameters.UpdateTimer)
        {
            rFirst->m_UpdateNext = pOther->m_UpdateNext;
            pOther->m_UpdateNext = rFirst;
            return;
        }
        pOther = pOther->m_UpdateNext.get();
    }
    pOther->m_UpdateNext = rFirst;
    rFirst->m_UpdateNext = nullptr;
}


void OrbitSystem2D::HandleOrbiterEscapingHost(NodeRef& node)
{
    LV_PROFILE_FUNCTION();

    // If true anomaly is less than true anomaly of escape, orbiter has not escaped its host's influence;
    // if true anomaly is greater than pi, orbiter is still inside the influence and is approaching periapsis.
    if (node->Parameters.TrueAnomaly < node->Parameters.TrueAnomalyEscape
        || node->Parameters.TrueAnomaly > LV::PIf)
    {
        return;
    }

    auto& op = node->Parameters;

    auto oldHost = node->Parent;
    node->Parent = oldHost->Parent;

    // Compute state relative to new host
    op.GravAsOrbiter = oldHost->Parent->Parameters.GravAsHost;
    op.Position = oldHost->Parameters.Position + (op.Position * oldHost->Influence.Radius);
    op.Velocity = oldHost->Parameters.Velocity + (op.Velocity * oldHost->Influence.Radius);

    // Recompute parameters and update orbit tree:
    node->ComputeElementsFromState();
    if (node->Influencing)
    {
        auto inflNode = std::static_pointer_cast<InfluencingNode>(node);

        // Remove node from old parent's children
        auto childrenOrbiterIt = std::find(oldHost->InfluencingChildren.begin(), oldHost->InfluencingChildren.end(), inflNode);
        size_t childrenLastIdx = oldHost->InfluencingChildren.size() - 1;
        childrenOrbiterIt->swap(oldHost->InfluencingChildren[childrenLastIdx]);
        oldHost->InfluencingChildren.resize(childrenLastIdx);
        // Add node to new parent's children
        node->Parent->InfluencingChildren.push_back(inflNode);

        // Compute influence with new host
        inflNode->ComputeInfluence();
    }
    else
    {
        // Remove node from old parent's children
        auto childrenOrbiterIt = std::find(oldHost->NonInflChildren.begin(), oldHost->NonInflChildren.end(), node);
        size_t childrenLastIdx = oldHost->NonInflChildren.size() - 1;
        childrenOrbiterIt->swap(oldHost->NonInflChildren[childrenLastIdx]);
        oldHost->NonInflChildren.resize(childrenLastIdx);
        // Add node to new parent's children
        node->Parent->NonInflChildren.push_back(node);
    }

    // Update sibling intersects
    RemoveOrbiterIntersectsFromSiblings(node, oldHost);

    m_OrbiterChangedHostCallback(node->Id);
}


void OrbitSystem2D::HandleOrbiterOverlappingInfluence(NodeRef& node)
{
    LV_PROFILE_FUNCTION();

    // Test if this orbiter is overlapped by the circle of influence of any orbiters of the same host
    auto& op = node->Parameters;
    for (auto& other : node->Parent->InfluencingChildren)
    {
        if (node == other) continue; // Skip self

        // Check overlap
        auto rPosition = op.Position - other->Parameters.Position;
        float separation = sqrt(rPosition.SqrMagnitude());
        if (separation > other->Influence.Radius)
        {
            continue;
        }

        // Overlap confirmed
        LV_CORE_INFO("Overlap: orbiter {0} -> influence {1}!", node->Id, other->Id);

        auto oldHost = node->Parent;
        node->Parent = other;

        // Compute state relative to new host
        op.GravAsOrbiter = other->Parameters.GravAsHost;
        op.Position = rPosition / other->Influence.Radius;
        op.Velocity = (op.Velocity - other->Parameters.Velocity) / other->Influence.Radius;

        // Recompute parameters and update orbit tree:
        node->ComputeElementsFromState();
        if (node->Influencing)
        {
            auto inflNode = std::static_pointer_cast<InfluencingNode>(node);

            // Remove node from old parent's children
            auto childrenOrbiterIt = std::find(oldHost->InfluencingChildren.begin(), oldHost->InfluencingChildren.end(), inflNode);
            size_t childrenLastIdx = oldHost->InfluencingChildren.size() - 1;
            childrenOrbiterIt->swap(oldHost->InfluencingChildren[childrenLastIdx]);
            oldHost->InfluencingChildren.resize(childrenLastIdx);
            // Add node to new parent's children
            other->InfluencingChildren.push_back(inflNode);

            // Compute influence with new host
            inflNode->ComputeInfluence();
        }
        else
        {
            // Remove node from old parent's children
            auto childrenOrbiterIt = std::find(oldHost->NonInflChildren.begin(), oldHost->NonInflChildren.end(), node);
            size_t childrenLastIdx = oldHost->NonInflChildren.size() - 1;
            childrenOrbiterIt->swap(oldHost->NonInflChildren[childrenLastIdx]);
            oldHost->NonInflChildren.resize(childrenLastIdx);
            // Add node to new parent's children
            other->NonInflChildren.push_back(node);
        }

        // Update sibling intersects
        RemoveOrbiterIntersectsFromSiblings(node, oldHost);

        m_OrbiterChangedHostCallback(node->Id);

        break;
    }
}


void OrbitSystem2D::RemoveOrbiterIntersectsFromSiblings(NodeRef& node, InflRef& parent)
{
    for (auto& sibling : parent->InfluencingChildren)
    {
        node->Parameters.Intersects.erase(sibling->Id);
        sibling->Parameters.Intersects.erase(node->Id);
    }
    for (auto& sibling : parent->NonInflChildren)
    {
        node->Parameters.Intersects.erase(sibling->Id);
        sibling->Parameters.Intersects.erase(node->Id);
    }
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

    m_AllNodes.clear();
    m_AllNodes.push_back(m_LevelHost);
    
    m_InfluencingNodes.clear();
    m_InfluencingNodes.insert({ 0, m_LevelHost });
}


uint32_t OrbitSystem2D::CreateOrbiterES(const bool influencing, const bool dynamic, const LV::BigFloat& mass, const uint32_t initialHostId, LV::Vector2 scaledPosition, LV::BigVector2 scaledVelocity)
{
    LV_PROFILE_FUNCTION();

    // Determine parent node (host of orbit)
    auto& p = FindLowestOverlappingInfluence(scaledPosition, scaledVelocity, initialHostId);

    return influencing ? CreateInfluencingNode(dynamic, p, mass, scaledPosition, scaledVelocity)
        : CreateNoninflNode(dynamic, p, mass, scaledPosition, scaledVelocity);
}


uint32_t OrbitSystem2D::CreateOrbiterCS(const bool influencing, const bool dynamic, const LV::BigFloat& mass, const uint32_t initialHostId, LV::Vector2 scaledPosition, const bool clockwise)
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

    return influencing ? CreateInfluencingNode(dynamic, p, mass, scaledPosition, scaledVelocity)
        : CreateNoninflNode(dynamic, p, mass, scaledPosition, scaledVelocity);
}


uint32_t OrbitSystem2D::CreateOrbiterEU(const bool influencing, const bool dynamic, const LV::BigFloat& mass, const LV::BigVector2& position, const LV::BigVector2& velocity)
{
    LV_PROFILE_FUNCTION();

    // Determine parent node (host of orbit)
    LV::Vector2 scaledPosition = (position * m_LevelHost->Influence.TotalScaling).Vector2();
    LV::BigVector2 scaledVelocity = velocity * m_LevelHost->Influence.TotalScaling;
    auto& p = FindLowestOverlappingInfluence(scaledPosition, scaledVelocity);

    return influencing ? CreateInfluencingNode(dynamic, p, mass, scaledPosition, scaledVelocity)
        : CreateNoninflNode(dynamic, p, mass, scaledPosition, scaledVelocity);
}


uint32_t OrbitSystem2D::CreateOrbiterCU(const bool influencing, const bool dynamic, const LV::BigFloat& mass, const LV::BigVector2& position, const bool clockwise)
{
    LV_PROFILE_FUNCTION();

    LV::Vector2 scaledPosition = (position * m_LevelHost->Influence.TotalScaling).Vector2();

    return CreateOrbiterCS(influencing, dynamic, mass, 0, scaledPosition, clockwise);
}


uint32_t OrbitSystem2D::CreateInfluencingNode(const bool dynamic, const InflRef& parent, const LV::BigFloat& mass, const LV::Vector2& scaledPosition, const LV::BigVector2& scaledVelocity)
{
    LV_PROFILE_FUNCTION();

    InfluencingNode* newNode = new InfluencingNode;
    newNode->Id = m_AllNodes.size(); // NOT SAFE - orbiter deletion may result in subsequent creations having duplicate IDs
    newNode->Parent = parent;

    // Compute gravitational properties of system
    newNode->Mass = mass;
    auto& op = newNode->Parameters;
    op.GravAsOrbiter = parent->Parameters.GravAsHost; // mu = GM -> Assumes mass of orbiter is insignificant compared to host

    // Compute orbital elements
    op.Position = scaledPosition;
    op.Velocity = scaledVelocity;
    newNode->Dynamic = dynamic;
    newNode->NeedRecomputeState = false;
    newNode->ComputeElementsFromState();

    // Compute this orbiter's influence
    newNode->ComputeInfluence();

    // Add to tree
    auto inflRef = InflRef(newNode);
    auto orbRef = std::static_pointer_cast<OrbitTreeNode>(inflRef);
    m_AllNodes.push_back(orbRef);
    m_InfluencingNodes.insert({ newNode->Id, inflRef });
    newNode->Parent->InfluencingChildren.push_back(inflRef);
    newNode->m_UpdateNext = m_UpdateFirst;
    m_UpdateFirst = orbRef;


    // debug //
    m_DebugData.emplace(newNode->Id, std::make_shared<Limnova::CsvTable<float, uint32_t, float, float, float>>());
    m_DebugData[newNode->Id].Table->Init(
        "Orbiter Debug Data: Orbiter " + std::to_string(newNode->Id),
        "OrbiterDebugData/orbiter" + std::to_string(newNode->Id) + ".txt",
        { "T (s)", "Num.Passes", "Predicted Pass Time(s)", "Actual Pass Time(s)", "Error(ms)" }, false
    );
    m_UpdateCounts[newNode->Id] = 0;
    // debug //


    return newNode->Id;
}


uint32_t OrbitSystem2D::CreateNoninflNode(const bool dynamic, const InflRef& parent, const Limnova::BigFloat& mass, const Limnova::Vector2& scaledPosition, const Limnova::BigVector2& scaledVelocity)
{
    LV_PROFILE_FUNCTION();

    OrbitTreeNode* newNode = new OrbitTreeNode;
    newNode->Id = m_AllNodes.size(); // NOT SAFE - orbiter deletion may result in subsequent creations having duplicate IDs
    newNode->Parent = parent;

    // Compute gravitational properties of system
    newNode->Mass = mass;
    auto& op = newNode->Parameters;
    op.GravAsOrbiter = parent->Parameters.GravAsHost; // mu = GM -> Assumes mass of orbiter is insignificant compared to host

    // Compute orbital elements
    op.Position = scaledPosition;
    op.Velocity = scaledVelocity;
    newNode->Dynamic = dynamic;
    newNode->NeedRecomputeState = false;
    newNode->ComputeElementsFromState();

    // Add to tree
    auto orbRef = NodeRef(newNode);
    m_AllNodes.push_back(orbRef);
    newNode->Parent->NonInflChildren.push_back(orbRef);
    newNode->m_UpdateNext = m_UpdateFirst;
    m_UpdateFirst = orbRef;

    return newNode->Id;
}


Limnova::Vector2 OrbitSystem2D::OrbitTreeNode::GetPosition(const float trueAnomaly)
{
    LV_PROFILE_FUNCTION();

    auto& op = this->Parameters;
    float sinT = sin(trueAnomaly);
    float cosT = cos(trueAnomaly);
    return op.OParameter
        * (op.BasisX * cosT + op.BasisY * sinT)
        / (1.f + op.Eccentricity * cosT);
}


bool OrbitSystem2D::OrbitTreeNode::StepTrueAnomalyIntegration(const float gameDeltaTime)
{
    LV_PROFILE_FUNCTION();

    if (this->Parameters.UpdateTimer > 0) return false;
    //if (this->Parameters.UpdateTimer > gameDeltaTime) return false;

    auto& op = this->Parameters;

    // dTAnom / dT = h / r^2 --> dT_optimal = dTAnom_optimal * r^2 / h
    float simDeltaTAnomaly = kMinimumDeltaTAnom;
    float simDeltaTime = ((kMinimumDeltaTAnom * op.Position.SqrMagnitude()) / op.OSAMomentum).Float();

    // TEMPORARY - never allows multiple updates per frame
    // TODO - allow multiple updates to maintain integration accuracy for faster orbiters (and while speeding-up time)
    //if (simDeltaTime < gameDeltaTime)
    //{
    //    // dTAnom = dT * h / r^2
    //    simDeltaTime = gameDeltaTime;
    //    simDeltaTAnomaly = gameDeltaTime * op.OSAMomentum.Float() / op.Position.SqrMagnitude();
    //}

    op.TrueAnomaly += simDeltaTAnomaly;
    op.UpdateTimer += simDeltaTime;

    return true;
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
    if (e2 > 1.f)
    {
        LV_CORE_ASSERT(this->Dynamic, "Static orbits cannot be hyperbolic trajectories - they must be circular or elliptical!");

        op.Type = OrbitType::Hyperbola;
        op.Eccentricity = sqrtf(e2);
        op.BasisX = eVec.Normalized();
    }
    else if (e2 > 0.f)
    {
        op.Type = OrbitType::Ellipse;
        op.Eccentricity = sqrtf(e2);
        op.BasisX = eVec.Normalized();
    }
    else
    {
        op.Type = OrbitType::Circle;
        op.Eccentricity = 0;
        op.BasisX = ur;
    }
    op.BasisY = op.CcwF * LV::Vector2(-op.BasisX.y, op.BasisX.x);

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

    float E2term = op.Type == OrbitType::Hyperbola ? e2 - 1.f : 1.f - e2;
    op.SemiMajorAxis = op.OParameter / E2term;
    op.SemiMinorAxis = op.SemiMajorAxis * sqrtf(E2term);
    op.Centre = -op.SemiMajorAxis * op.Eccentricity * op.BasisX;
    if (op.Type == OrbitType::Hyperbola)
    {
        op.Centre *= -1.f;
    }

    op.Period = LV::PI2f * op.SemiMajorAxis * op.SemiMinorAxis / op.OSAMomentum;

    // Predicting orbit events:
    // If distance to apoapsis is greater than escape distance, or if the orbit is hyperbolic,
    // the orbiter will leave the host's influence:
    LV_CORE_ASSERT(this->Dynamic
        || this->Parent == s_OrbitSystem2D.m_LevelHost
        || op.OParameter / (1.f - op.Eccentricity) < kEscapeDistance, // r_a = h^2 / mu(1 - e)
        "Static orbits should not leave their host's influence!");

    if ((this->Dynamic
        && this->Parent != s_OrbitSystem2D.m_LevelHost
        && op.OParameter / (1.f - op.Eccentricity) > kEscapeDistance
        ) || op.Type == OrbitType::Hyperbola)
    {
        // Orbiter leaves host's influence at the point that its orbital distance is equal to the escape distance (r_esc):
        // cos(TAnomaly) = (h^2 / (mu * r_esc) - 1) / e
        op.TrueAnomalyEscape = acosf((op.OParameter / kEscapeDistance - 1.f) / op.Eccentricity);
        LV_CORE_INFO("Orbiter {0} will escape {1} at true anomaly {2} (current true anomaly {3})", this->Id, this->Parent->Id, op.TrueAnomalyEscape, op.TrueAnomaly);

        // Determine orbit time from periapse to escape
        float meanAnomaly;
        float trueAnomalyTerm = op.Eccentricity * sqrt(E2term) * sinf(op.TrueAnomaly)
            / (1.f + op.Eccentricity * cosf(op.TrueAnomaly));
        float tanTerm = tanf(op.TrueAnomaly / 2.f);
        if (op.Type == OrbitType::Hyperbola)
        {
            float sqrtEplus1 = sqrt(op.Eccentricity + 1.f);
            float sqrtEminus1 = sqrt(op.Eccentricity - 1.f);
            meanAnomaly = trueAnomalyTerm -
                logf((sqrtEplus1 + sqrtEminus1 * tanTerm)
                    / (sqrtEplus1 - sqrtEminus1 * tanTerm));
        }
        else
        {
            meanAnomaly = 2.f * atanf(
                    sqrt((1.f - op.Eccentricity) / (1.f + op.Eccentricity))
                    * tanTerm
                ) - trueAnomalyTerm;
        }
        op.TimePeriapseToEscape = meanAnomaly * op.Period / LV::PI2f;

        float sinT = sin(op.TrueAnomalyEscape);
        float cosT = cos(op.TrueAnomalyEscape);
        // Point of escape relative to the host, oriented to the perifocal frame (y = 0 is the apse line)
        float r_escape = op.OParameter / (1.f + op.Eccentricity * cosT);
        op.EscapePointPerifocal = { cosT * r_escape, sinT * r_escape };

        // Points of entry and escape relative to the host, oriented to the scene
        op.EscapePointsScene[0] = op.OParameter
            * (op.BasisX * cosT + op.BasisY * sinT)
            / (1.f + op.Eccentricity * cosT);
        op.EscapePointsScene[1] = op.OParameter
            * (op.BasisX * cosT - op.BasisY * sinT)
            / (1.f + op.Eccentricity * cosT);
    }
    else
    {
        op.TrueAnomalyEscape = 2.f * LV::PI2f; // True anomaly can never exceed 4Pi - this orbiter will never pass the host-escape test
    }

    // Orbit intersects:
    op.Intersects.clear();
    // Simplest case, intersects only (ignores influences):
    for (auto& child : this->Parent->NonInflChildren)
    {
        if (child->Id == this->Id) { continue; }

        this->FindIntersects(child);
    }
    for (auto& child : this->Parent->InfluencingChildren)
    {
        if (child->Id == this->Id) { continue; }

        auto childBaseNode = std::static_pointer_cast<OrbitTreeNode>(child);
        this->FindIntersects(childBaseNode);

        // TODO - for influencing siblings, find points of influence overlap
    }

    // Complex case, detect nearest approaches which are closer than the radius of influence:
    // Find where this orbit intersects another by finding the minima of a function which computes the distance between the two points
    // on the orbits which sit on the same line from the shared focus (the points with the same right ascension relative to a shared frame of
    // reference). If a minimum is less than the other orbiter's radius of influence, it counts as an intersect.
    //for (auto& child : this->Parent->InfluencingChildren)
    //{
    //    if (child->Id == this->Id) { continue; }

    //    std::pair<uint32_t, float[2]> intersects;
    //    intersects.first = 0;

    //    auto& cp = child->Parameters;

    //    float rhoDelta = op.RightAscensionPeriapsis - cp.RightAscensionPeriapsis;

    //    auto func = [&](const float theta0) -> float
    //    {
    //        float theta1 = op.CcwF > 0 ? rhoDelta + theta0 : rhoDelta - theta0;
    //        return abs(cp.OParameter / (1.f + cp.Eccentricity * cosf(theta1)) - op.OParameter / (1.f + op.Eccentricity * cosf(theta0)));
    //    };

    //    static constexpr float baseStep = LV::PIover8f;
    //    float start = 0.f;
    //    float fPrev = func(start);
    //    bool prevDecreased = false;
    //    for (float theta = start + baseStep; theta < LV::PI2f; theta += baseStep)
    //    {
    //        float f = func(theta);
    //        if (f < fPrev)
    //        {
    //            prevDecreased = true;
    //        }
    //        else if (prevDecreased && f < child->Influence.Radius)
    //        {
    //            intersects.second[intersects.first] = theta;
    //            intersects.first++;
    //            if (intersects.first == 2) { break; }
    //            prevDecreased = false;
    //        }
    //        fPrev = f;
    //    }

    //    if (intersects.first > 0)
    //    {
    //        op.Intersects.emplace(child->Id, std::move(intersects));

    //        // debug
    //        std::ostringstream ioss;
    //        ioss << intersects.second[0];
    //        if (intersects.first == 2)
    //        {
    //            ioss << ", " << intersects.second[1];
    //        }
    //        LV_CORE_INFO("Orbiter {0} intersects {1}: {2}", this->Id, child->Id, ioss.str());
    //    }
    //}

    // TODO : prohibit an influencing orbiter from (potentially) overlapping another's influence

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


void OrbitSystem2D::OrbitTreeNode::FindIntersects(NodeRef& sibling)
{
    LV_PROFILE_FUNCTION();

    auto& op = this->Parameters;
    auto& sp = sibling->Parameters;

    // True anomaly theta of orbit i intersecting with orbit f, where f has an apse line rotated by angle eta (relative to i's perifocal frame):
    // theta = alpha +/- acos(c * cos(alpha) / a),
    // alpha = atan(b / a),
    // a = p_f * e_i - p_i * e_f * cos(eta),
    // b = -p_i * e_f * sin(eta),
    // c = p_i - p_f,
    // where p is orbital parameter and e is eccentricity.

    // Find eta = relative rotation of sibling's apse line
    float eta = op.CcwF > 0 ? sp.RightAscensionPeriapsis - op.RightAscensionPeriapsis
        : op.RightAscensionPeriapsis - sp.RightAscensionPeriapsis;
    if (eta < 0) { eta = LV::PI2f + eta; }

    float a = sp.OParameter * op.Eccentricity - op.OParameter * sp.Eccentricity * cosf(eta);
    float b = -op.OParameter * sp.Eccentricity * sinf(eta);
    float c = op.OParameter - sp.OParameter;
    float alpha = atanf(b / a);

    // Test if intersects are possible
    float cCosAlpha = c * cosf(alpha);
    if (abs(cCosAlpha) > abs(a))
    {
        op.Intersects[sibling->Id].first = 0;
        sp.Intersects[this->Id].first = 0;
        return;
    }

    // Compute intersects in this orbit
    float theta0 = LV::Wrap(alpha + acosf(cCosAlpha / a), -LV::PIf, LV::PIf);
    float theta1 = LV::Wrap(alpha - acosf(cCosAlpha / a), -LV::PIf, LV::PIf);

    // Compute intersects in sibling orbit
    float siblingTheta0 = op.RightAscensionPeriapsis - sp.RightAscensionPeriapsis + (op.CcwF > 0 ? theta0 : -theta0);
    float siblingTheta1 = op.RightAscensionPeriapsis - sp.RightAscensionPeriapsis + (op.CcwF > 0 ? theta1 : -theta1);
    if (sp.CcwF < 0)
    {
        siblingTheta0 = -siblingTheta0;
        siblingTheta1 = -siblingTheta1;
    }
    siblingTheta0 = LV::Wrap(siblingTheta0, -LV::PIf, LV::PIf);
    siblingTheta1 = LV::Wrap(siblingTheta1, -LV::PIf, LV::PIf);

    // For each intersect:
    // Add to both orbits if it is within both escape/entry points
    auto& intersect = op.Intersects[sibling->Id];
    intersect.first = 0;
    auto& siblingIntersect = sp.Intersects[this->Id];
    siblingIntersect.first = 0;

    // TODO : ability to update all sibling intersects when this orbiter escapes its host:
    // - use a central data structure in OrbitSystem2D to store intersects ?
    // - iterate over siblings and remove Intersect entries for this orbiter's ID ?

    int numIntersects = 0;
    if (abs(theta0) < op.TrueAnomalyEscape && abs(siblingTheta0) < sp.TrueAnomalyEscape)
    {
        intersect.second[numIntersects] = this->GetPosition(theta0);
        siblingIntersect.second[numIntersects] = sibling->GetPosition(siblingTheta0);

        numIntersects++;
    }
    if (abs(theta1) < op.TrueAnomalyEscape && abs(siblingTheta1) < sp.TrueAnomalyEscape)
    {
        intersect.second[numIntersects] = this->GetPosition(theta1);
        siblingIntersect.second[numIntersects] = sibling->GetPosition(siblingTheta1);

        numIntersects++;
    }
    intersect.first = numIntersects;
    siblingIntersect.first = numIntersects;
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
        LV_CORE_ERROR("Orbiter {0} mass ({1}) is too high to orbit influencing orbiter {2} ({3})!", this->Id, this->Mass, this->Parent->Id, this->Parent->Mass);
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

    LV_CORE_ASSERT(m_InfluencingNodes.find(initialHostId) != m_InfluencingNodes.end(), "Invalid orbiter ID!");

    uint32_t parentId = initialHostId;
    for (uint32_t i = 0; i < m_AllNodes.size(); i++)
    {
        auto& inflNode = FindOverlappingChildInfluence(m_InfluencingNodes[parentId], scaledPosition);
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


const OrbitSystem2D::OrbitTreeNode& OrbitSystem2D::GetOrbiter(const uint32_t orbiterId)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(orbiterId < m_AllNodes.size() && orbiterId >= 0, "Invalid orbiter ID!");

    auto& node = m_AllNodes[orbiterId];
    if (node->NeedRecomputeState)
    {
        node->ComputeStateVector();
        node->NeedRecomputeState = false;
    }
    return *node;
}


const OrbitSystem2D::InfluencingNode& OrbitSystem2D::GetHost(const uint32_t hostId)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(m_InfluencingNodes.find(hostId) != m_InfluencingNodes.end(), "Invalid orbiter ID!");
    
    auto& node = m_InfluencingNodes[hostId];
    if (node->NeedRecomputeState)
    {
        node->ComputeStateVector();
        node->NeedRecomputeState = false;
    }
    return *node;
}


const OrbitSystem2D::OrbitParameters& OrbitSystem2D::GetParameters(const uint32_t orbiterId)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(orbiterId < m_AllNodes.size() && orbiterId >= 0, "Invalid orbiter ID!");

    auto& node = m_AllNodes[orbiterId];
    if (node->NeedRecomputeState)
    {
        node->ComputeStateVector();
        node->NeedRecomputeState = false;
    }
    return m_AllNodes[orbiterId]->Parameters;
}


float OrbitSystem2D::GetRadiusOfInfluence(const uint32_t orbiterId)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(m_InfluencingNodes.find(orbiterId) != m_InfluencingNodes.end(), "Invalid orbiter ID!");

    return m_InfluencingNodes[orbiterId]->Influence.Radius;
}


float OrbitSystem2D::GetScaling(const uint32_t hostId)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(m_InfluencingNodes.find(hostId) != m_InfluencingNodes.end(), "Invalid orbiter ID!");

    return m_InfluencingNodes[hostId]->Influence.TotalScaling.Float();
}


float OrbitSystem2D::GetHostScaling(const uint32_t orbiterId)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(orbiterId < m_AllNodes.size() && orbiterId > 0, "Invalid orbiter ID!");

    return m_AllNodes[orbiterId]->Parent->Influence.TotalScaling.Float();
}


uint32_t OrbitSystem2D::GetOrbiterHost(const uint32_t orbiterId)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(orbiterId < m_AllNodes.size() && orbiterId > 0, "Invalid orbiter ID!");

    return m_AllNodes[orbiterId]->Parent->Id;
}


bool OrbitSystem2D::IsInfluencing(const uint32_t orbiterId)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(orbiterId < m_AllNodes.size() && orbiterId >= 0, "Invalid orbiter ID!");

    return m_AllNodes[orbiterId]->Influencing;
}


void OrbitSystem2D::GetOrbiters(const uint32_t hostId, std::vector<uint32_t>& childIds)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(m_InfluencingNodes.find(hostId) != m_InfluencingNodes.end(), "OrbitSystem2D::GetOrbiters() was passed an invalid host orbiter ID ({0})!");

    for (auto& child : m_InfluencingNodes[hostId]->InfluencingChildren)
    {
        childIds.push_back(child->Id);
    }
    for (auto& child : m_InfluencingNodes[hostId]->NonInflChildren)
    {
        childIds.push_back(child->Id);
    }
}


void OrbitSystem2D::SetOrbiterRightAscension(const uint32_t orbiterId, const float rightAscension)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(orbiterId < m_AllNodes.size() && orbiterId > 0, "Invalid orbiter ID!");

    auto& op = m_AllNodes[orbiterId]->Parameters;

    op.TrueAnomaly = op.CcwF > 0 ? rightAscension - op.RightAscensionPeriapsis : op.RightAscensionPeriapsis - rightAscension;
    if (op.TrueAnomaly < 0)
    {
        op.TrueAnomaly += LV::PI2f;
    }

    m_AllNodes[orbiterId]->NeedRecomputeState = true;
}


void OrbitSystem2D::GetAllHosts(std::vector<uint32_t>& ids)
{
    LV_PROFILE_FUNCTION();

    for (auto& infl : m_InfluencingNodes)
    {
        ids.push_back(infl.first);
    }
}


void OrbitSystem2D::SetTimeScale(const float timescale)
{
    m_Timescale = timescale;
    m_MinimumDeltaT = m_Timescale * kMinimumDeltaT;
}


void OrbitSystem2D::RecordData()
{
    for (uint32_t n = 1; n < m_AllNodes.size(); n++)
    {
        m_DebugData[n].Table->Write();
    }
}
