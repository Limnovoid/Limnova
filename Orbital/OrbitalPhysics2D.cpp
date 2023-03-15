#include "OrbitalPhysics2D.h"

#include <chrono>


namespace LV = Limnova;

static const LV::BigFloat kGrav = { 6.6743f, -11 };
static constexpr float kMinimumDeltaTAnom = 1e-4f;
static constexpr uint32_t kMaxUpdatesPerNodePerFrame = 20;
static constexpr float kMinimumDeltaT = 1.f / (60.f * (float)kMaxUpdatesPerNodePerFrame); // maximum 20 updates per node per frame at 60 frames per second
static constexpr float kMinimumNewtonStep = 1e-6f;
static constexpr float kEscapeDistance = 1.01f;
static constexpr float kMinimumAxisRatioSqrt = 1e-2f; // Minimum ratio of semi-minor to semi-major axis for a well-defined orbit
static constexpr float kEpsilonE2term = 1e-4f;
static constexpr float kEpsilonEccentricity = 1e-2f;


OrbitalPhysics2D::OrbitalPhysics2D()
{
    m_MinimumDeltaT = m_Timescale * kMinimumDeltaT;
}


OrbitalPhysics2D::~OrbitalPhysics2D()
{
}


OrbitalPhysics2D OrbitalPhysics2D::s_OrbitSystem2D;

void OrbitalPhysics2D::Init()
{
    LV_PROFILE_FUNCTION();

    s_OrbitSystem2D.m_NumNodesAllocated = 0;
    s_OrbitSystem2D.m_FreeNodes.clear();
    s_OrbitSystem2D.m_FreeInflNodes.clear();

    s_OrbitSystem2D.m_SystemHost.reset();
    s_OrbitSystem2D.m_AllNodes.clear();
    s_OrbitSystem2D.m_InfluencingNodes.clear();
    s_OrbitSystem2D.m_DynamicNodes.clear();


    // debug - orbiter integration accuracy
    s_OrbitSystem2D.m_DebugData.clear();
}


OrbitalPhysics2D& OrbitalPhysics2D::Get()
{
    return s_OrbitSystem2D;
}


void OrbitalPhysics2D::Shutdown()
{
    LV_PROFILE_FUNCTION();

    if (s_OrbitSystem2D.m_Testing)
    {
        s_OrbitSystem2D.RecordData();
    }
}


void OrbitalPhysics2D::Update(Limnova::Timestep dT)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(m_UpdateFirst.get() != nullptr, "Update queue head pointer is null!");


    std::ostringstream debugoss; // debug
    debugoss << "Node update counts:\n"; // debug


    float gameDeltaTime = m_Timescale * (float)dT;

    // TEMPORARY - apply all non-gravitational accelerations
    for (auto& node : m_DynamicNodes)
    {
        auto& op = node.second->Parameters;

        if (op.DynamicAcceleration.IsZero())
        {
            continue;
        }

        // Recompute orbits after applying acceleration
        op.Velocity += gameDeltaTime * op.DynamicAcceleration;
        node.second->ComputeElementsFromState();

        // If orbit type has become a linear trajectory, prepare for Newtonian integration
        if (node.second->Parameters.NewtonianMotion)
        {
            m_UpdateFirst->ComputeGravityAccelerationFromState();
        }

        op.DynamicAcceleration = LV::BigVector2::Zero();

        // Update timer is not affected:
        // Assumes per-frame acceleration is not enough to significantly change the optimal time to next update
        // TODO - test this assumption

        // TODO - explore moving the integration step into the main update loop
    }

    // Update all orbit nodes:
    // Nodes are queued in ascending order of their individual times until next update (stored in OrbitParameters::UpdateTimer),
    // which is measured from the start of the current frame - when a node is updated, its UpdateTimer increases by the size of
    // its individual timestep. The queue is iterated through in order until all UpdateTimers are greater than the gameDeltaTime.
    // This allows nodes to be updated with different time steps, zero or more times per frame (for each node), while still
    // updating them all chronologically for more accurate collision tracking.
    while (m_UpdateFirst->Parameters.UpdateTimer < gameDeltaTime)
    {
        m_UpdateCounts[m_UpdateFirst->Id]++; // debug


        auto& op = m_UpdateFirst->Parameters;


        // Different integration methods for different types of motion:
        // Both integration methods must add a nodeDT to UpdateTimer,
        // so we do this outside the scopes of the separate methods.
        float nodeDeltaTime;

        float r2 = op.Position.SqrMagnitude();
        float prevTrueAnomaly = op.TrueAnomaly;

        if (op.NewtonianMotion)
        {
            // Newtonian integration for ill-defined orbits:
            nodeDeltaTime = sqrt(kMinimumNewtonStep / sqrt(op.Acceleration.SqrMagnitude().Float()));

            // TODO - ASSERT that nodeDeltaTime computed above is a reasonable approximation of
            // the timestep required to produce a change in position equal to kMinimumNewtonStep

            // Limit number of steps per frame
            nodeDeltaTime = gameDeltaTime / nodeDeltaTime < kMaxUpdatesPerNodePerFrame
                ? nodeDeltaTime : gameDeltaTime / (float)kMaxUpdatesPerNodePerFrame;

            // Velocity Verlet
            op.Position = op.Position + op.Velocity * nodeDeltaTime + 0.5f * op.Acceleration * powf(nodeDeltaTime, 2.f);
            LV::BigVector2 newAcceleration = -LV::BigVector2(op.Position) * (op.GravAsOrbiter / (r2 * sqrt(r2)));
            op.Velocity = op.Velocity + 0.5f * (op.Acceleration + newAcceleration) * nodeDeltaTime;
            op.Acceleration = newAcceleration;

            op.TrueAnomaly = op.Position.SqrMagnitude() / powf(kEscapeDistance, 2.f) * op.TrueAnomalyEscape;

            op.DynamicAcceleration = LV::BigVector2::Zero();
        }
        else
        {
            // True anomaly integration for well-defined orbits:
            nodeDeltaTime = ((kMinimumDeltaTAnom * r2) / op.OSAMomentum).Float();
            float nodeDeltaTAnomaly = kMinimumDeltaTAnom;

            if (nodeDeltaTime < m_MinimumDeltaT)
            {
                // Limit number of updates per node per frame - see kMinimumDeltaT
                nodeDeltaTime = m_MinimumDeltaT;
                nodeDeltaTAnomaly = (m_MinimumDeltaT * op.OSAMomentum / r2).Float();
            }

            op.TrueAnomaly += nodeDeltaTAnomaly;
            if (op.TrueAnomaly > LV::PI2f)
            {
                op.TrueAnomaly -= LV::PI2f;
            }

            m_UpdateFirst->ComputeStateVector();
        }
        op.UpdateTimer += nodeDeltaTime;

        // Handle orbit events:
        if (m_UpdateFirst->Dynamic)
        {
            HandleOrbiterEscapingHost(m_UpdateFirst);
            HandleOrbiterOverlappingInfluence(m_UpdateFirst);

            // Check if intersects have been passed in this update
            for (auto& intersect : op.Intersects)
            {
                for (uint32_t i = 0; i < intersect.second.Count; i++)
                {
                    if (prevTrueAnomaly < intersect.second.TrueAnomalies[i] 
                        && op.TrueAnomaly > intersect.second.TrueAnomalies[i])
                    {
                        intersect.second.NeedComputeOtherOrbiterPositions[i] = true;
                    }
                }
            }
        }

        SortUpdateFirst();
    }
    // Per-frame updates are now complete for all orbit nodes: subtract gameDeltaTime from all UpdateTimers.
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

    std::cout << debugoss.str() << std::endl; // debug
}


void OrbitalPhysics2D::SortUpdateFirst()
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


void OrbitalPhysics2D::HandleOrbiterEscapingHost(NodeRef& node)
{
    LV_PROFILE_FUNCTION();

    // If true anomaly is less than true anomaly of escape, orbiter has not escaped its host's influence;
    // if true anomaly is greater than pi, orbiter is still inside the influence and is approaching periapsis.
    if (node->Parameters.TrueAnomaly < node->Parameters.TrueAnomalyEscape
        || node->Parameters.TrueAnomaly > LV::PIf)
    {
        return;
    }

    if (node->Parent == m_SystemHost)
    {
        LV_CORE_WARN("Orbiter {0} escaped the level and was destroyed!", node->Id);
        m_OrbiterDestroyedCallback(node->Id);
        RemoveNodeFromUpdateQueue(node); // In case the callback is missing or does not call DestroyNode()
        // RemoveNodeFromUpdateQueue() is safe to call on nodes which are not in the queue - it will print a WARN if that is the case
    }

    // Escape confirmed
    auto& op = node->Parameters;

    auto oldHost = node->Parent;

    // Compute state relative to new host
    op.GravAsOrbiter = oldHost->Parent->Parameters.GravAsHost;
    op.Position = oldHost->Parameters.Position + (op.Position * oldHost->Influence.Radius);
    op.Velocity = oldHost->Parameters.Velocity + (op.Velocity * oldHost->Influence.Radius);
    op.Acceleration *= oldHost->Influence.Radius;

    // Update node references
    ChangeNodeParent(node, oldHost, oldHost->Parent);
    RemoveOrbiterIntersectsFromSiblings(node, oldHost);

    // Recompute parameters and influence
    node->ComputeElementsFromState();
    if (node->Influencing)
    {
        std::static_pointer_cast<InfluencingNode>(node)->ComputeInfluence();
    }

    m_OrbiterChangedHostCallback(node->Id, true);
}


void OrbitalPhysics2D::HandleOrbiterOverlappingInfluence(NodeRef& node)
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
        LV_CORE_ASSERT(other->Id != node->Parent->Id, "Orbiter (re-)overlapped its parent's influence!");
        LV_CORE_INFO("Overlap: orbiter {0} -> influence {1}!", node->Id, other->Id);

        // Compute state relative to new host
        op.GravAsOrbiter = other->Parameters.GravAsHost;
        op.Position = rPosition / other->Influence.Radius;
        op.Velocity = (op.Velocity - other->Parameters.Velocity) / other->Influence.Radius;
        op.Acceleration /= other->Influence.Radius;

        // Update node references
        auto oldHost = node->Parent;
        ChangeNodeParent(node, oldHost, other);
        RemoveOrbiterIntersectsFromSiblings(node, oldHost);

        // Recompute parameters and influence
        node->ComputeElementsFromState();
        if (node->Influencing)
        {
            std::static_pointer_cast<InfluencingNode>(node)->ComputeInfluence();
        }

        m_OrbiterChangedHostCallback(node->Id, false);

        break;
    }
}


void OrbitalPhysics2D::RemoveOrbiterIntersectsFromSiblings(NodeRef& node, InflRef& parent)
{
    LV_PROFILE_FUNCTION();

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


void OrbitalPhysics2D::ChangeNodeParent(NodeRef& node, InflRef& oldParent, InflRef& newParent)
{
    node->Parent = newParent;

    if (node->Influencing)
    {
        auto inflNode = std::static_pointer_cast<InfluencingNode>(node);

        // Remove node from old parent's children
        auto childrenOrbiterIt = std::find(oldParent->InfluencingChildren.begin(), oldParent->InfluencingChildren.end(), inflNode);
        LV_CORE_ASSERT(childrenOrbiterIt != oldParent->InfluencingChildren.end(), "Could not find influencing node in its parent's InfluenceChildren vector!");
        oldParent->InfluencingChildren.erase(childrenOrbiterIt);

        // Add node to new parent's children
        newParent->InfluencingChildren.push_back(inflNode);
    }
    else
    {
        // Remove node from old parent's children
        auto childrenOrbiterIt = std::find(oldParent->NonInflChildren.begin(), oldParent->NonInflChildren.end(), node);
        LV_CORE_ASSERT(childrenOrbiterIt != oldParent->NonInflChildren.end(), "Could not find non-influencing node in its parent's NonInflChildren vector!");
        oldParent->NonInflChildren.erase(childrenOrbiterIt);

        // Add node to new parent's children
        newParent->NonInflChildren.push_back(node);
    }
}


void OrbitalPhysics2D::RemoveNodeFromUpdateQueue(NodeRef& node)
{
    LV_PROFILE_FUNCTION();

    if (node == m_UpdateFirst)
    {
        m_UpdateFirst = node->m_UpdateNext;
    }
    else
    {
        OrbitTreeNode* updatePrev = m_UpdateFirst.get();
        while (updatePrev->m_UpdateNext != node)
        {
            if (updatePrev->m_UpdateNext == nullptr)
            {
                LV_CORE_WARN("Node ({0}) passed to RemoveNodeFromUpdateQueue() was not found in the queue!", node->Id);
                node->m_UpdateNext = nullptr;
                return;
            }

            updatePrev = updatePrev->m_UpdateNext.get();
        }
        updatePrev->m_UpdateNext = node->m_UpdateNext;
    }
    node->m_UpdateNext = nullptr;
}


uint32_t OrbitalPhysics2D::LoadLevel(const LV::BigFloat& hostMass, const LV::BigFloat& baseScaling)
{
    LV_PROFILE_FUNCTION();

    m_NumNodesAllocated = 0;
    m_FreeNodes.clear();
    m_FreeInflNodes.clear();

    m_SystemHost = GetFreeInflNode();
    m_SystemHost->Id = 0;
    m_SystemHost->Mass = hostMass;
    m_SystemHost->Parameters.GravAsOrbiter = kGrav * hostMass;
    m_SystemHost->Influence.TotalScaling = baseScaling;

    // Length dimension in G (the gravitational constant) is cubed - scaling must be cubed when computing scaled-GM
    m_SystemHost->Parameters.GravAsHost = m_SystemHost->Parameters.GravAsOrbiter / LV::BigFloat::Pow(baseScaling, 3);

    m_AllNodes.clear();
    m_AllNodes[0] = m_SystemHost;

    m_InfluencingNodes.clear();
    m_InfluencingNodes[0] = m_SystemHost;

    m_DynamicNodes.clear();

    return m_SystemHost->Id;
}


void OrbitalPhysics2D::SetTimeScale(const float timescale)
{
    LV_PROFILE_FUNCTION();

    m_Timescale = timescale;
    m_MinimumDeltaT = m_Timescale * kMinimumDeltaT;
}


uint32_t OrbitalPhysics2D::CreateOrbiterES(const bool influencing, const bool dynamic, const LV::BigFloat& mass, const uint32_t initialHostId, LV::Vector2 scaledPosition, LV::BigVector2 scaledVelocity)
{
    LV_PROFILE_FUNCTION();

    // Determine parent node (host of orbit)
    auto& p = FindLowestOverlappingInfluence(scaledPosition, scaledVelocity, initialHostId);

    return influencing ? CreateInfluencingOrbiter(dynamic, p, mass, scaledPosition, scaledVelocity)
        : CreateNoninflOrbiter(dynamic, p, mass, scaledPosition, scaledVelocity);
}


uint32_t OrbitalPhysics2D::CreateOrbiterCS(const bool influencing, const bool dynamic, const LV::BigFloat& mass, const uint32_t initialHostId, LV::Vector2 scaledPosition, const bool clockwise)
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

    uint32_t id = influencing ? CreateInfluencingOrbiter(dynamic, p, mass, scaledPosition, scaledVelocity)
        : CreateNoninflOrbiter(dynamic, p, mass, scaledPosition, scaledVelocity);
    LV_CORE_ASSERT(m_AllNodes[id]->Parameters.Type == OrbitType::Circle, "Circular orbit creator function produced non-circular orbit parameters!");
    return id;
}


uint32_t OrbitalPhysics2D::CreateOrbiterEU(const bool influencing, const bool dynamic, const LV::BigFloat& mass, const LV::BigVector2& position, const LV::BigVector2& velocity)
{
    LV_PROFILE_FUNCTION();

    // Determine parent node (host of orbit)
    LV::Vector2 scaledPosition = (position * m_SystemHost->Influence.TotalScaling).Vector2();
    LV::BigVector2 scaledVelocity = velocity * m_SystemHost->Influence.TotalScaling;
    auto& p = FindLowestOverlappingInfluence(scaledPosition, scaledVelocity);

    return influencing ? CreateInfluencingOrbiter(dynamic, p, mass, scaledPosition, scaledVelocity)
        : CreateNoninflOrbiter(dynamic, p, mass, scaledPosition, scaledVelocity);
}


uint32_t OrbitalPhysics2D::CreateOrbiterCU(const bool influencing, const bool dynamic, const LV::BigFloat& mass, const LV::BigVector2& position, const bool clockwise)
{
    LV_PROFILE_FUNCTION();

    LV::Vector2 scaledPosition = (position * m_SystemHost->Influence.TotalScaling).Vector2();

    uint32_t id = CreateOrbiterCS(influencing, dynamic, mass, 0, scaledPosition, clockwise);
    LV_CORE_ASSERT(m_AllNodes[id]->Parameters.Type == OrbitType::Circle, "Circular orbit creator function produced non-circular orbit parameters!");
    return id;
}


OrbitalPhysics2D::NodeRef OrbitalPhysics2D::GetFreeNode()
{
    LV_PROFILE_FUNCTION();

    if (m_FreeNodes.size() > 0)
    {
        auto it = m_FreeNodes.begin();
        auto nodeRef = *it;
        m_FreeNodes.erase(it);
        return nodeRef;
    }

    return std::make_shared<OrbitTreeNode>(m_NumNodesAllocated++);
}


OrbitalPhysics2D::InflRef OrbitalPhysics2D::GetFreeInflNode()
{
    LV_PROFILE_FUNCTION();

    if (m_FreeInflNodes.size() > 0)
    {
        auto it = m_FreeInflNodes.begin();
        auto inflRef = *it;
        m_FreeInflNodes.erase(it);
        return inflRef;
    }

    return std::make_shared<InfluencingNode>(m_NumNodesAllocated++);
}


void OrbitalPhysics2D::SetNodeFree(NodeRef& node)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(m_FreeNodes.find(node) == m_FreeNodes.end(), "Node ID is already free!");

    m_FreeNodes.insert(node);
}


void OrbitalPhysics2D::SetInflNodeFree(InflRef& inflNode)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(m_FreeInflNodes.find(inflNode) == m_FreeInflNodes.end(), "Node ID is already free!");

    m_FreeInflNodes.insert(inflNode);
}


uint32_t OrbitalPhysics2D::CreateInfluencingOrbiter(const bool dynamic, const InflRef& parent, const LV::BigFloat& mass, const LV::Vector2& scaledPosition, const LV::BigVector2& scaledVelocity)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(scaledPosition.SqrMagnitude() > 0, "Position cannot be zero!");

    InflRef inflRef = GetFreeInflNode();
    inflRef->Parent = parent;

    // Compute gravitational properties of system
    inflRef->Mass = mass;
    auto& op = inflRef->Parameters;
    op.GravAsOrbiter = parent->Parameters.GravAsHost; // mu = GM -> Assumes mass of orbiter is insignificant compared to host

    // Compute orbital elements
    op.Position = scaledPosition;
    op.Velocity = scaledVelocity;
    inflRef->Dynamic = dynamic;
    inflRef->ComputeElementsFromState();

    // Compute this orbiter's influence
    inflRef->ComputeInfluence();

    // Add to data structures
    auto orbRef = std::static_pointer_cast<OrbitTreeNode>(inflRef);
    m_AllNodes[inflRef->Id] = orbRef;
    m_InfluencingNodes[inflRef->Id] = inflRef;
    if (dynamic)
    {
        m_DynamicNodes[inflRef->Id] = orbRef;
    }
    inflRef->Parent->InfluencingChildren.push_back(inflRef);
    inflRef->m_UpdateNext = m_UpdateFirst;
    m_UpdateFirst = orbRef;


    // debug //
    m_DebugData.emplace(inflRef->Id, std::make_shared<Limnova::CsvTable<float, uint32_t, float, float, float>>());
    m_DebugData[inflRef->Id].Table->Init(
        "Orbiter Debug Data: Orbiter " + std::to_string(inflRef->Id),
        "OrbiterDebugData/orbiter" + std::to_string(inflRef->Id) + ".txt",
        { "T (s)", "Num.Passes", "Predicted Pass Time(s)", "Actual Pass Time(s)", "Error(ms)" }, false
    );
    m_UpdateCounts[inflRef->Id] = 0;
    // debug //


    return inflRef->Id;
}


uint32_t OrbitalPhysics2D::CreateNoninflOrbiter(const bool dynamic, const InflRef& parent, const Limnova::BigFloat& mass, const Limnova::Vector2& scaledPosition, const Limnova::BigVector2& scaledVelocity)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(scaledPosition.SqrMagnitude() > 0, "Position cannot be zero!");

    NodeRef nodeRef = GetFreeNode();
    nodeRef->Parent = parent;

    // Compute gravitational properties of system
    nodeRef->Mass = mass;
    auto& op = nodeRef->Parameters;
    op.GravAsOrbiter = parent->Parameters.GravAsHost; // mu = GM -> Assumes mass of orbiter is insignificant compared to host

    // Compute orbital elements
    op.Position = scaledPosition;
    op.Velocity = scaledVelocity;
    nodeRef->Dynamic = dynamic;
    nodeRef->ComputeElementsFromState();

    // Add to data structures
    m_AllNodes[nodeRef->Id] = nodeRef;
    if (dynamic)
    {
        m_DynamicNodes[nodeRef->Id] = nodeRef;
    }
    nodeRef->Parent->NonInflChildren.push_back(nodeRef);
    nodeRef->m_UpdateNext = m_UpdateFirst;
    m_UpdateFirst = nodeRef;

    return nodeRef->Id;
}


Limnova::Vector2 OrbitalPhysics2D::OrbitTreeNode::ComputePositionAtTrueAnomaly(const float trueAnomaly)
{
    LV_PROFILE_FUNCTION();

    float sinT = sin(trueAnomaly);
    float cosT = cos(trueAnomaly);
    return Parameters.OParameter
        * (Parameters.BasisX * cosT + Parameters.BasisY * sinT)
        / (1.f + Parameters.Eccentricity * cosT);
}


Limnova::Vector2 OrbitalPhysics2D::OrbitTreeNode::GetOtherOrbiterPositionAtIntersect(const uint32_t otherOrbiterId, const uint32_t intersect)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(Parameters.Intersects.find(otherOrbiterId) != Parameters.Intersects.end()
        && intersect < Parameters.Intersects[otherOrbiterId].Count, "Intersect does not exist!");

    auto& is = Parameters.Intersects[otherOrbiterId];

    if (is.NeedComputeOtherOrbiterPositions[intersect])
    {
        // Compute time to next intersect
        LV::BigFloat timeOfCurrentPosition = FindTimeOfTrueAnomaly(Parameters.TrueAnomaly);
        LV::BigFloat timeOfIntersect = FindTimeOfTrueAnomaly(is.TrueAnomalies[intersect]);
        LV::BigFloat timeToNextIntersection = LV::WrapBf(timeOfIntersect - timeOfCurrentPosition, LV::BigFloat::Zero, Parameters.Period);

        auto& otherOrbiter = s_OrbitSystem2D.m_AllNodes[otherOrbiterId];
        float otherOrbiterTrueAnomalyAtNextIntersection = otherOrbiter->FindFutureTrueAnomaly(timeToNextIntersection);
        is.OtherOrbiterPositions[intersect] = otherOrbiter->ComputePositionAtTrueAnomaly(otherOrbiterTrueAnomalyAtNextIntersection);

        is.NeedComputeOtherOrbiterPositions[intersect] = false;
    }
    return is.OtherOrbiterPositions[intersect];
}


void OrbitalPhysics2D::OrbitTreeNode::ComputeElementsFromState()
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
    else if (e2 - kEpsilonEccentricity > 0.f)
    {
        op.Type = OrbitType::Ellipse;
        op.Eccentricity = sqrtf(e2);
        op.BasisX = eVec.Normalized();
    }
    else
    {
        op.Type = OrbitType::Circle;
        op.Eccentricity = 0.f;
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
    op.muh = op.OSAMomentum.IsZero() ? 0.f : op.GravAsOrbiter / op.OSAMomentum;

    float E2term = op.Type == OrbitType::Hyperbola ? e2 - 1.f : 1.f - e2;
    if (E2term < kEpsilonE2term)
    {
        E2term = kEpsilonE2term;
    }
    op.SemiMajorAxis = op.OParameter / E2term;
    op.SemiMinorAxis = op.SemiMajorAxis * sqrtf(E2term);
    op.Centre = -op.SemiMajorAxis * op.Eccentricity * op.BasisX;
    if (op.Type == OrbitType::Hyperbola)
    {
        op.Centre *= -1.f;
    }

    // Detect orbits which are too steep for true anomaly-based integration
    if (E2term < kMinimumAxisRatioSqrt)
    {
        op.NewtonianMotion = true;

        this->ComputeGravityAccelerationFromState();

        op.TrueAnomaly = op.Position.SqrMagnitude() / powf(kEscapeDistance, 2.f) * op.TrueAnomalyEscape;

        LV_CORE_WARN("Orbit is too steep for integration of true-anomaly!");
    }
    else
    {
        op.NewtonianMotion = false;
    }

    op.Period = LV::PI2f * op.SemiMajorAxis * op.SemiMinorAxis / op.OSAMomentum;

    LV_CORE_ASSERT(this->Dynamic
        || op.OParameter / (1.f - op.Eccentricity) < kEscapeDistance,
        "Static orbits should not leave their host's influence!");

    // Predicting orbit events:
    // If distance to apoapsis is greater than escape distance, or if the orbit is hyperbolic,
    // the orbiter will leave the host's influence: r_a = p / (1 - e)
    if ((this->Dynamic && op.OParameter / (1.f - op.Eccentricity) > kEscapeDistance)
        || op.Type == OrbitType::Hyperbola)
    {
        // Orbiter leaves host's influence at the point that its orbital distance is equal to the escape distance (r_esc):
        // cos(TAnomaly) = (h^2 / (mu * r_esc) - 1) / e
        op.TrueAnomalyEscape = acosf((op.OParameter / kEscapeDistance - 1.f) / op.Eccentricity);
        LV_CORE_INFO("Orbiter {0} will escape {1} at true anomaly {2} (current true anomaly {3})", this->Id, this->Parent->Id, op.TrueAnomalyEscape, op.TrueAnomaly);
        LV_CORE_ASSERT(op.TrueAnomaly < op.TrueAnomalyEscape || op.TrueAnomaly > LV::PIf, "Orbiter true anomaly is in its computed escape range at the time of computing the escape true anomaly!");

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
            * (op.BasisX * cosT + op.BasisY * sinT) // +Uy
            / (1.f + op.Eccentricity * cosT);
        op.EscapePointsScene[1] = op.OParameter
            * (op.BasisX * cosT - op.BasisY * sinT) // -Uy
            / (1.f + op.Eccentricity * cosT);
    }
    else if (op.NewtonianMotion)
    {
        op.TrueAnomalyEscape = LV::PIf - kMinimumDeltaTAnom;
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


void OrbitalPhysics2D::OrbitTreeNode::ComputeStateVector()
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


void OrbitalPhysics2D::OrbitTreeNode::ComputeGravityAccelerationFromState()
{
    LV_PROFILE_FUNCTION();

    auto& op = this->Parameters;

    float r2 = op.Position.SqrMagnitude();
    op.Acceleration = -LV::BigVector2(op.Position) * (op.GravAsOrbiter / (r2 * sqrt(r2)));
}


void OrbitalPhysics2D::OrbitTreeNode::FindIntersects(NodeRef& sibling)
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
        sp.Intersects[this->Id].Count = 0;
        op.Intersects[sibling->Id].Count = 0;
        return;
    }

    // Compute intersects in this orbit
    float theta0 = LV::Wrapf(alpha + acosf(cCosAlpha / a), -LV::PIf, LV::PIf);
    float theta1 = LV::Wrapf(alpha - acosf(cCosAlpha / a), -LV::PIf, LV::PIf);

    // Compute intersects in sibling orbit
    float siblingTheta0 = op.RightAscensionPeriapsis - sp.RightAscensionPeriapsis + (op.CcwF > 0 ? theta0 : -theta0);
    float siblingTheta1 = op.RightAscensionPeriapsis - sp.RightAscensionPeriapsis + (op.CcwF > 0 ? theta1 : -theta1);
    if (sp.CcwF < 0)
    {
        siblingTheta0 = -siblingTheta0;
        siblingTheta1 = -siblingTheta1;
    }
    siblingTheta0 = LV::Wrapf(siblingTheta0, -LV::PIf, LV::PIf);
    siblingTheta1 = LV::Wrapf(siblingTheta1, -LV::PIf, LV::PIf);

    // For each intersect:
    // Add to both orbits if it is within both escape/entry points
    auto& intersect = op.Intersects[sibling->Id];
    intersect.OtherOrbiterId = sibling->Id;
    intersect.Count = 0;
    auto& siblingIntersect = sp.Intersects[this->Id];
    siblingIntersect.OtherOrbiterId = this->Id;
    siblingIntersect.Count = 0;

    int numIntersects = 0;
    if (abs(theta0) < op.TrueAnomalyEscape && abs(siblingTheta0) < sp.TrueAnomalyEscape)
    {
        intersect.TrueAnomalies[numIntersects] = theta0;
        intersect.Positions[numIntersects] = this->ComputePositionAtTrueAnomaly(theta0);
        intersect.NeedComputeOtherOrbiterPositions[numIntersects] = true;

        siblingIntersect.TrueAnomalies[numIntersects] = siblingTheta0;
        siblingIntersect.Positions[numIntersects] = sibling->ComputePositionAtTrueAnomaly(siblingTheta0);
        siblingIntersect.NeedComputeOtherOrbiterPositions[numIntersects] = true;

        numIntersects++;
    }
    if (abs(theta1) < op.TrueAnomalyEscape && abs(siblingTheta1) < sp.TrueAnomalyEscape)
    {
        intersect.TrueAnomalies[numIntersects] = theta1;
        intersect.Positions[numIntersects] = this->ComputePositionAtTrueAnomaly(theta1);
        intersect.NeedComputeOtherOrbiterPositions[numIntersects] = true;

        siblingIntersect.TrueAnomalies[numIntersects] = siblingTheta1;
        siblingIntersect.Positions[numIntersects] = sibling->ComputePositionAtTrueAnomaly(siblingTheta1);
        siblingIntersect.NeedComputeOtherOrbiterPositions[numIntersects] = true;

        numIntersects++;
    }
    intersect.Count = numIntersects;
    siblingIntersect.Count = numIntersects;
}


LV::BigFloat OrbitalPhysics2D::OrbitTreeNode::FindTimeOfTrueAnomaly(const float trueAnomaly)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(Parameters.Type == OrbitType::Circle || Parameters.Type == OrbitType::Ellipse, "FindTimeOfTrueAnomaly() currently only supports orbits with eccentricity < 1!");

    float eccentricAnomaly = 2.f * atanf(sqrt((1.f - Parameters.Eccentricity) / (1.f + Parameters.Eccentricity)) * tanf(0.5f * trueAnomaly));
    float meanAnomaly = eccentricAnomaly - Parameters.Eccentricity * sinf(eccentricAnomaly);
    return meanAnomaly / LV::PI2f * Parameters.Period;
}


float OrbitalPhysics2D::OrbitTreeNode::FindFutureTrueAnomaly(const LV::BigFloat& deltaTime)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(Parameters.Type == OrbitType::Circle || Parameters.Type == OrbitType::Ellipse, "FindFutureTrueAnomaly() currently only supports orbits with eccentricity < 1!");

    LV::BigFloat timeAtTrueAnomaly = LV::WrapBf(FindTimeOfTrueAnomaly(Parameters.TrueAnomaly) + deltaTime, LV::BigFloat::Zero, Parameters.Period);
    float meanAnomaly = LV::PI2f * (timeAtTrueAnomaly / Parameters.Period).Float();

    // Infinite series solution
    float eccentricAnomaly = meanAnomaly;
    for (uint32_t n = 1; n < 10; n++)
    {
        float J = 0.f, x = (float)n * Parameters.Eccentricity;
        for (uint32_t k = 0; k < 10; k++)
        {
            J += powf(-1.f, k) * powf(0.5f * x, n + k) / (float)(LV::Factorial(k) * LV::Factorial(n + k));
        }
        eccentricAnomaly += 2.f * J * sinf((float)n * meanAnomaly) / (float)n;
    }
    float trueAnomaly = 2.f * atanf(tanf(0.5f * eccentricAnomaly) / sqrt((1.f - Parameters.Eccentricity) / (1.f + Parameters.Eccentricity)));

#ifdef LV_DEBUG
    auto timeOfPredictedTrueAnomaly = FindTimeOfTrueAnomaly(trueAnomaly);
    auto predictionError = (timeOfPredictedTrueAnomaly - timeAtTrueAnomaly).Float();
    LV_CORE_ASSERT(abs(predictionError) < kEpsilonEccentricity, "FindFutureTrueAnomaly() could not calculate true anomaly to less than kEpsilonEccentricity!");
#endif

    return trueAnomaly;
}


void OrbitalPhysics2D::InfluencingNode::ComputeInfluence()
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


OrbitalPhysics2D::InflRef& OrbitalPhysics2D::FindLowestOverlappingInfluence(LV::Vector2& scaledPosition, Limnova::BigVector2& scaledVelocity, const uint32_t initialHostId)
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
    return m_SystemHost;
}


OrbitalPhysics2D::InflRef& OrbitalPhysics2D::FindOverlappingChildInfluence(InflRef& parent, const LV::Vector2& scaledPosition)
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


void OrbitalPhysics2D::DestroyOrbiter(const uint32_t orbiterId)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(orbiterId < m_AllNodes.size() && orbiterId >= 0, "Invalid orbiter ID!");

    NodeRef nodeRef = m_AllNodes[orbiterId];

    LV_CORE_ASSERT(!nodeRef->Influencing, "Influencing nodes cannot be destroyed (at this point in development)!");

    // Remove from update queue
    RemoveNodeFromUpdateQueue(nodeRef);

    // Remove from parent's children
    auto childrenOrbiterIt = std::find(nodeRef->Parent->NonInflChildren.begin(), nodeRef->Parent->NonInflChildren.end(), nodeRef);
    LV_CORE_ASSERT(childrenOrbiterIt != nodeRef->Parent->NonInflChildren.end(), "Could not find non-influencing node in its parent's NonInflChildren vector!");
    nodeRef->Parent->NonInflChildren.erase(childrenOrbiterIt);

    // Remove from sibling intersects
    RemoveOrbiterIntersectsFromSiblings(nodeRef, nodeRef->Parent);

    // Remove from structures
    LV_CORE_ASSERT(m_AllNodes.find(nodeRef->Id) != m_AllNodes.end(), "Node does not have an existing reference!");
    m_AllNodes.erase(nodeRef->Id);
    if (nodeRef->Dynamic)
    {
        LV_CORE_ASSERT(m_DynamicNodes.find(nodeRef->Id) != m_DynamicNodes.end(), "Dynamic node does not have an existing reference!");
        m_DynamicNodes.erase(nodeRef->Id);
    }
    if (nodeRef->Influencing)
    {
        LV_CORE_ASSERT(m_InfluencingNodes.find(nodeRef->Id) != m_InfluencingNodes.end(), "Node does not have an existing reference!");
        m_InfluencingNodes.erase(nodeRef->Id);

        // Free node so it can be re-used
        auto inflNode = std::static_pointer_cast<InfluencingNode>(nodeRef);
        SetInflNodeFree(inflNode);
    }
    else
    {
        SetNodeFree(nodeRef);
    }
}


const OrbitalPhysics2D::OrbitTreeNode& OrbitalPhysics2D::GetOrbiter(const uint32_t orbiterId)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(m_AllNodes.find(orbiterId) != m_AllNodes.end(), "Invalid orbiter ID!");

    auto& node = m_AllNodes[orbiterId];
    return *node;
}


const OrbitalPhysics2D::InfluencingNode& OrbitalPhysics2D::GetHost(const uint32_t hostId)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(m_InfluencingNodes.find(hostId) != m_InfluencingNodes.end(), "Invalid orbiter ID!");
    
    auto& node = m_InfluencingNodes[hostId];
    return *node;
}


const OrbitalPhysics2D::OrbitParameters& OrbitalPhysics2D::GetParameters(const uint32_t orbiterId)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(m_AllNodes.find(orbiterId) != m_AllNodes.end(), "Invalid orbiter ID!");

    auto& node = m_AllNodes[orbiterId];
    return m_AllNodes[orbiterId]->Parameters;
}


uint32_t OrbitalPhysics2D::GetHostId(const uint32_t orbiterId)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(m_AllNodes.find(orbiterId) != m_AllNodes.end() && m_AllNodes[orbiterId]->Parent != nullptr, "Invalid orbiter ID!");

    return m_AllNodes[orbiterId]->Parent->Id;
}


float OrbitalPhysics2D::GetRadiusOfInfluence(const uint32_t orbiterId)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(m_InfluencingNodes.find(orbiterId) != m_InfluencingNodes.end(), "Invalid orbiter ID!");

    return m_InfluencingNodes[orbiterId]->Influence.Radius;
}


float OrbitalPhysics2D::GetScaling(const uint32_t hostId)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(m_InfluencingNodes.find(hostId) != m_InfluencingNodes.end(), "Invalid orbiter ID!");

    return m_InfluencingNodes[hostId]->Influence.TotalScaling.Float();
}


float OrbitalPhysics2D::GetHostScaling(const uint32_t orbiterId)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(m_AllNodes.find(orbiterId) != m_AllNodes.end(), "Invalid orbiter ID!");

    return m_AllNodes[orbiterId]->Parent->Influence.TotalScaling.Float();
}


uint32_t OrbitalPhysics2D::GetOrbiterHost(const uint32_t orbiterId)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(orbiterId > 0 && m_AllNodes.find(orbiterId) != m_AllNodes.end(), "Invalid orbiter ID!");

    return m_AllNodes[orbiterId]->Parent->Id;
}


bool OrbitalPhysics2D::IsInfluencing(const uint32_t orbiterId)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(m_AllNodes.find(orbiterId) != m_AllNodes.end(), "Invalid orbiter ID!");

    return m_AllNodes[orbiterId]->Influencing;
}


void OrbitalPhysics2D::GetOrbiters(const uint32_t hostId, std::vector<uint32_t>& childIds)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(m_InfluencingNodes.find(hostId) != m_InfluencingNodes.end(), "OrbitalPhysics2D::GetOrbiters() was passed an invalid host orbiter ID ({0})!");

    for (auto& child : m_InfluencingNodes[hostId]->InfluencingChildren)
    {
        childIds.push_back(child->Id);
    }
    for (auto& child : m_InfluencingNodes[hostId]->NonInflChildren)
    {
        childIds.push_back(child->Id);
    }
}


void OrbitalPhysics2D::SetOrbiterRightAscension(const uint32_t orbiterId, const float rightAscension)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(orbiterId > 0 && m_AllNodes.find(orbiterId) != m_AllNodes.end(), "Invalid orbiter ID!");

    auto& op = m_AllNodes[orbiterId]->Parameters;

    op.TrueAnomaly = op.CcwF > 0 ? rightAscension - op.RightAscensionPeriapsis : op.RightAscensionPeriapsis - rightAscension;
    if (op.TrueAnomaly < 0)
    {
        op.TrueAnomaly += LV::PI2f;
    }

    m_AllNodes[orbiterId]->ComputeStateVector();
}


void OrbitalPhysics2D::GetAllHosts(std::vector<uint32_t>& ids)
{
    LV_PROFILE_FUNCTION();

    for (auto& infl : m_InfluencingNodes)
    {
        ids.push_back(infl.first);
    }
}


void OrbitalPhysics2D::AccelerateOrbiter(const uint32_t orbiterId, const Limnova::BigVector2& accelaration)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(m_DynamicNodes.find(orbiterId) != m_DynamicNodes.end(), "AccelerateOrbiter() was passed an invalid orbiter ID!");

    LV::BigVector2 scaledAcceleration = accelaration * m_DynamicNodes[orbiterId]->Parent->Influence.TotalScaling;
    m_DynamicNodes[orbiterId]->Parameters.DynamicAcceleration += scaledAcceleration;
}


OrbitalPhysics2D::NodeRef& OrbitalPhysics2D::GetNodeRef(const uint32_t orbiterId)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(m_AllNodes.find(orbiterId) != m_AllNodes.end(), "Invalid orbiter ID!");

    return m_AllNodes[orbiterId];
}


OrbitalPhysics2D::InflRef& OrbitalPhysics2D::GetInflRef(const uint32_t orbiterId)
{
    LV_PROFILE_FUNCTION();

    LV_CORE_ASSERT(m_InfluencingNodes.find(orbiterId) != m_InfluencingNodes.end(), "AccelerateOrbiter() was passed an invalid orbiter ID!");

    return m_InfluencingNodes[orbiterId];
}


OrbitalPhysics2D::OrbitParameters OrbitalPhysics2D::ComputeOrbit(const uint32_t hostId, const LV::Vector2& scaledPosition, const LV::BigVector2& scaledVelocity)
{
    LV_PROFILE_FUNCTION();

    OrbitTreeNode tempNode(std::numeric_limits<uint32_t>::max());

    LV_CORE_ASSERT(m_InfluencingNodes.find(hostId) != m_InfluencingNodes.end(), "ComputeParameters() was passed an invalid host ID!");
    tempNode.Parent = m_InfluencingNodes[hostId];
    tempNode.Parameters.GravAsOrbiter = tempNode.Parent->Parameters.GravAsHost;
    tempNode.Parameters.Position = scaledPosition;
    tempNode.Parameters.Velocity = scaledVelocity;
    tempNode.Dynamic = true;
    tempNode.ComputeElementsFromState();
    return tempNode.Parameters;
}


void OrbitalPhysics2D::RecordData()
{
    for (uint32_t n = 1; n < m_AllNodes.size(); n++)
    {
        m_DebugData[n].Table->Write();
    }
}