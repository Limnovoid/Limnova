#pragma once

#include <Limnova.h>


class OrbitalPhysics2D
{
private:
    static OrbitalPhysics2D s_OrbitSystem2D;
public:
    class OrbitTreeNode;
    class InfluencingNode;
    using NodeRef = std::shared_ptr<OrbitTreeNode>;
    using InflRef = std::shared_ptr<InfluencingNode>;


    enum class OrbitType
    {
        Circle      = 0,
        Ellipse     = 1,
        Hyperbola   = 2
    };


    struct Intersect
    {
        uint32_t OtherOrbiterId;
        uint32_t Count = 0;
        float TrueAnomalies[2];
        Limnova::Vector2 Positions[2];

        friend class OrbitalPhysics2D;
    private:
        bool NeedComputeOtherOrbiterPositions[2];
        Limnova::Vector2 OtherOrbiterPositions[2]; // The positions of the other orbiter at the next times that this orbiter crosses the intersect
    };


    struct OrbitParameters
    {
        Limnova::BigFloat GravAsHost = { 0.f, 0 }; // Gravitational parameter of this orbiter, used by its own children for computing their orbits around this host = mass * GravitationalConstant
        Limnova::BigFloat GravAsOrbiter = { 0.f, 0 }; // Gravitation parameter of this orbiter's host, used for this orbiter's own computations = hostmass * GravitationalConstant

        // State - scaled to host's radius of influence
        Limnova::Vector2 Position           = { 0.f, 0.f };
        Limnova::BigVector2 Velocity        = { 0.f, 0.f };
        Limnova::BigVector2 Acceleration    = { 0.f, 0.f };
        float UpdateTimer = 0.f;

        // Perifocal frame
        Limnova::Vector2 BasisX = { 1.f, 0.f };
        Limnova::Vector2 BasisY = { 0.f, 1.f };
        Limnova::Vector2 Centre = { 0.f, 0.f };
        float RightAscensionPeriapsis = 0.f;

        // Elements
        Limnova::BigFloat OSAMomentum = 0.f; // Orbital specific angular momentum
        float OParameter = 0.f; // Orbit parameter = h^2 / mu
        float Eccentricity = 0.f;
        float TrueAnomaly = 0.f;
        OrbitType Type;

        // Dimensions
        float SemiMajorAxis = 0.f;
        float SemiMinorAxis = 0.f;
        Limnova::BigFloat Period = 0.f;
        float CcwF = 1.f; // Counter-clockwise factor: 1 for a counter-clockwise orbit, -1 for a clockwise orbit

        // Computation constants
        Limnova::BigFloat muh = 0.f; // mu / h

        // Dynamics
        float TrueAnomalyEscape = 2.f * Limnova::PI2f; // True anomaly of the (first) position on the orbit with an orbital distance equal to the host's ROI (1 for scaled orbits); if no such positions exist, this variable is set to 4pi (an impossible value for true anomaly).
        // TrueAnomalyEscape is not necessary for engineered, fixed orbits
        // TODO ?? : separate structs for different types of orbiter
        Limnova::BigFloat TimePeriapseToEscape = 0.f;
        Limnova::Vector2 EscapePointPerifocal;
        Limnova::Vector2 EscapePointsScene[2];
        Limnova::BigVector2 DynamicAcceleration = { 0.f, 0.f };
        bool NewtonianMotion = false;

        // For each other orbit which intersects this orbit, this member maps the ID of the other orbiter to the relevant intersect data: data is stored as a pair in which 'first' stores the number of intersects (0, 1, or 2), and 'second' stores their position vectors as an array of size 2.
        std::unordered_map<uint32_t, Intersect> Intersects;
    };


    class OrbitTreeNode
    {
        friend class OrbitalPhysics2D;
    public:
        OrbitTreeNode(const uint32_t id) : Id(id) {}
        virtual ~OrbitTreeNode() {}

        const uint32_t GetId() { return Id; }
        const OrbitParameters& GetParameters() const { return Parameters; }
        float GetHostScaling() const { return Parent->Influence.TotalScaling.Float(); }
        uint32_t GetHost() const { return Parent->Id; }

        Limnova::Vector2 ComputePositionAtTrueAnomaly(const float trueAnomaly);

        Limnova::Vector2 GetOtherOrbiterPositionAtIntersect(const uint32_t otherOrbiterId, const uint32_t intersect);
    protected:
        uint32_t Id;
        InflRef Parent = nullptr;

        Limnova::BigFloat Mass;
        OrbitParameters Parameters;
        bool Influencing = false;
        bool Dynamic = false;
    protected:
        void ComputeElementsFromState();
        void ComputeStateVector();
        void ComputeGravityAccelerationFromState();

        void FindIntersects(NodeRef& sibling);
        Limnova::BigFloat FindTimeOfTrueAnomaly(const float trueAnomaly);
        float FindFutureTrueAnomaly(const Limnova::BigFloat& deltaTime);
    protected:
        NodeRef m_UpdateNext = nullptr;
    };


    struct Influence
    {
        Limnova::BigFloat TotalScaling; // Use to multiply children's parameters to convert them from unscaled-absolute dimensions to relative-scaled dimensions.
        float Radius = 0.f; // Scaled by parent - use to multiply children's parameters to convert them from this influence's scale to the parent scale (move from this influence to the next-higher influence in the level).
    };


    class InfluencingNode : public OrbitTreeNode
    {
        friend class OrbitalPhysics2D;
    public:
        InfluencingNode(const uint32_t id) : OrbitTreeNode(id) { Influencing = true; }
        ~InfluencingNode() {}

        float GetScaling() const { return Influence.TotalScaling.Float(); }
        float GetRadiusOfInfluence() const { return Influence.Radius; }
        void GetChildren(std::vector<uint32_t>& ids) const
        {
            for (auto& child : InfluencingChildren)
            {
                ids.push_back(child->Id);
            }
            for (auto& child : NonInflChildren)
            {
                ids.push_back(child->Id);
            }
        }
    private:
        Influence Influence;
        std::vector<InflRef> InfluencingChildren;
        std::vector<NodeRef> NonInflChildren;
    private:
        void ComputeInfluence();
    };
public:
    OrbitalPhysics2D();
    ~OrbitalPhysics2D();

    static void Init();
    static OrbitalPhysics2D& Get();
    static void Shutdown();

    // Callback parameters:
    // uint32_t - ID of orbiter whose host has changed
    // bool - true if orbiter changed host by escaping old host's influence, false otherwise
    void SetOrbiterChangedHostCallback(const std::function<void(const uint32_t, const bool)> fn) { m_OrbiterChangedHostCallback = fn; };

    void SetOrbiterDestroyedCallback(const std::function<void(const uint32_t)> fn) { m_OrbiterDestroyedCallback = fn; };

    void Update(Limnova::Timestep dT);

    /// <summary>
    /// Initialise the orbit system by specifying the mass of the system host and the scaling ratio of the top-level orbit space.
    /// </summary>
    /// <param name="hostMass">Mass of system host</param>
    /// <param name="baseScaling">Real-world distance by which distances in the top-level orbit space are normalised:
    /// baseScale = 1 / realDistance.
    /// Object positions are normalised by the conversion ratio of their orbit host to make the measurements used in
    /// internal computations more manageable; most objects will have a separation from their host between 0 and 1.
    /// baseScaling is the ratio of the top-level host: its value converts measurements in the top-level orbit space to
    /// real-world measurements.</param>
    uint32_t LoadLevel(const Limnova::BigFloat& hostMass, const Limnova::BigFloat& baseScaling);

    // TODO : load level from file; save level to file

    void SetTimeScale(const float timescale);

    /// <summary>
    /// Explicit, Scaled: create an orbiter with given mass and an initial state vector scaled to the specified host,
    /// and place in orbit around the host whose gravitational influence dominates the initial position.
    /// CreateOrbiter returns ID of created orbiter - use ID to get render info with GetParameters.
    /// </summary>
    /// <param name="mass">Mass of orbiter</param>
    /// <param name="dynamic">Whether the orbit is dynamic - is able to change during gameplay</param>
    /// <param name="initialHostId">ID of the host to whose influence scaledPosition and scaledVelocity are scaled</param>
    /// <param name="scaledPosition">Initial position of orbiter, scaled to the top-level orbit space</param>
    /// <param name="scaledVelocity">Initial velocity of orbiter, scaled to the top-level orbit space</param>
    /// <returns>ID of created orbiter, or numeric_limits::unit32_t::max() if creation fails</returns>
    uint32_t CreateOrbiterES(const bool influencing, const bool dynamic, const Limnova::BigFloat& mass, const uint32_t initialHostId, Limnova::Vector2 scaledPosition, Limnova::BigVector2 scaledVelocity);

    /// <summary>
    /// Circular, Scaled: create an orbiter with given mass and an initial position scaled to the specified host, and place in a circular orbit
    /// which is clockwise or counter-clockwise as specified, around the host whose gravitational influence dominates the initial position.
    /// CreateOrbiter returns ID of created orbiter - use ID to get render info with GetParameters.
    /// </summary>
    /// <param name="mass">Mass of orbiter</param>
    /// <param name="scaledPosition">Initial position of orbiter, scaled to the top-level orbit space</param>
    /// <param name="clockwise">Whether the circular orbit is clockwise or counter-clockwise</param>
    /// <returns>ID of created orbiter, or numeric_limits::unit32_t::max() if creation fails</returns>
    uint32_t CreateOrbiterCS(const bool influencing, const bool dynamic, const Limnova::BigFloat& mass, const uint32_t initialHostId, Limnova::Vector2 scaledPosition, const bool clockwise = false);

    /// <summary>
    /// Explicit, Scaled: create an orbiter with given mass and an initial state vector,
    /// and place in orbit around the host whose gravitational influence dominates the initial position.
    /// CreateOrbiter returns ID of created orbiter - use ID to get render info with GetParameters.
    /// </summary>
    /// <param name="mass">Mass of orbiter</param>
    /// <param name="position">Initial position of orbiter (actual, unscaled)</param>
    /// <param name="velocity">Initial velocity of orbiter (actual, unscaled)</param>
    /// <returns>ID of created orbiter, or numeric_limits::unit32_t::max() if creation fails</returns>
    uint32_t CreateOrbiterEU(const bool influencing, const bool dynamic, const Limnova::BigFloat& mass, const Limnova::BigVector2& position, const Limnova::BigVector2& velocity);

    /// <summary>
    /// Circular, Unscaled: create an orbiter with given mass and initial position, and place in a circular orbit
    /// which is clockwise or counter-clockwise as specified, around the host whose gravitational influence dominates the initial position.
    /// CreateOrbiter returns ID of created orbiter - use ID to get render info with GetParameters.
    /// </summary>
    /// <param name="mass">Mass of orbiter</param>
    /// <param name="position">Initial position of orbiter (actual, unscaled)</param>
    /// <param name="clockwise">Whether the circular orbit is clockwise or counter-clockwise</param>
    /// <returns>ID of created orbiter, or numeric_limits::unit32_t::max() if creation fails</returns>
    uint32_t CreateOrbiterCU(const bool influencing, const bool dynamic, const Limnova::BigFloat& mass, const Limnova::BigVector2& position, const bool clockwise = false);

    //uint32_t CreateOrbiter(const Limnova::BigFloat& mass, const Limnova::Vector2& position, float eccentricity, float trueAnomaly = 0.f, bool clockwise = false);

    void DestroyOrbiter(const uint32_t orbiterId);

    const OrbitTreeNode& GetOrbiter(const uint32_t orbiterId);
    const InfluencingNode& GetHost(const uint32_t hostId);
    const OrbitParameters& GetParameters(const uint32_t orbiterId);
    uint32_t GetHostId(const uint32_t orbiterId);
    float GetRadiusOfInfluence(const uint32_t orbiterId);
    float GetScaling(const uint32_t hostId);
    float GetHostScaling(const uint32_t orbiterId);
    uint32_t GetOrbiterHost(const uint32_t orbiterId);
    bool IsInfluencing(const uint32_t orbiterId);
    /// <summary>
    /// Append the IDs of all orbiters which are orbiting the host with ID = hostId to the given vector.
    /// Ordering of IDs is arbitrary.
    /// </summary>
    void GetOrbiters(const uint32_t hostId, std::vector<uint32_t>& childIds);
    /// <summary>
    /// Ordering of results is arbitrary.
    /// </summary>
    void GetAllHosts(std::vector<uint32_t>& ids);

    void SetOrbiterRightAscension(const uint32_t orbiterId, const float rightAscension);

    void AccelerateOrbiter(const uint32_t orbiterId, const Limnova::BigVector2& acceleration);

    NodeRef& GetNodeRef(const uint32_t orbiterId);
    InflRef& GetInflRef(const uint32_t orbiterId);

    OrbitParameters ComputeOrbit(const uint32_t hostId, const Limnova::Vector2& scaledPosition, const Limnova::BigVector2& scaledVelocity);
private:
    uint32_t m_NumNodesAllocated;
    std::unordered_set<NodeRef> m_FreeNodes;
    std::unordered_set<InflRef> m_FreeInflNodes;

    InflRef m_SystemHost;
    std::unordered_map<uint32_t, NodeRef> m_AllNodes;
    std::unordered_map<uint32_t, InflRef> m_InfluencingNodes;
    std::unordered_map<uint32_t, NodeRef> m_DynamicNodes;
    NodeRef m_UpdateFirst = nullptr;

    float m_Timescale = 1.f;
    float m_MinimumDeltaT;

    // Callback parameters:
    // uint32_t - ID of orbiter whose host has changed
    // bool - true if orbiter changed host by escaping old host's influence, false otherwise
    std::function<void(const uint32_t /*ID*/, const bool /*escaped*/)> m_OrbiterChangedHostCallback;

    // Callback parameters:
    // uint32_t - ID of orbiter which has been destroyed
    std::function<void(const uint32_t /*ID*/)> m_OrbiterDestroyedCallback;
private:
    NodeRef GetFreeNode(); // Returns a reference to an unused node - assigns it a valid ID before returning
    InflRef GetFreeInflNode(); // Returns a reference to an unused influencing node - assigns it a valid before returning
    void SetNodeFree(NodeRef& node);
    void SetInflNodeFree(InflRef& inflNode);

    uint32_t CreateInfluencingOrbiter(const bool dynamic, const InflRef& parent, const Limnova::BigFloat& mass, const Limnova::Vector2& scaledPosition, const Limnova::BigVector2& scaledVelocity);

    uint32_t CreateNoninflOrbiter(const bool dynamic, const InflRef& parent, const Limnova::BigFloat& mass, const Limnova::Vector2& scaledPosition, const Limnova::BigVector2& scaledVelocity);

    /// <summary>
    /// Returns lowest-level node whose circle-of-influence overlaps the given position;
    /// converts the referenced position and velocity vectors to that node's orbit space.
    /// If no overlaps are found, returns m_LevelHost without modifying the vectors.
    /// </summary>
    /// <param name="scaledPosition">The position to check for overlaps and update to any overlapping node's orbit space.
    /// NOTE: must be initially scaled to the top-level orbit space (the scaling of m_LevelHost)</param>
    /// <param name="scaledVelocity">The velocity to update to any overlapping node's orbit space.
    /// NOTE: must be initially scaled to the top-level orbit space (the scaling of m_LevelHost)</param>
    InflRef& FindLowestOverlappingInfluence(Limnova::Vector2& scaledPosition, Limnova::BigVector2& scaledVelocity, const uint32_t initialHostId = 0);
    // Returns lowest-level node whose circle-of-influence overlaps the given absolue position, or the given parent if none overlaps.
    InflRef& FindOverlappingChildInfluence(InflRef& parent, const Limnova::Vector2& scaledPosition);

    // Sorts m_UpdateFirst in the update queue - assumes only the first item in the queue may have been altered.
    void SortUpdateFirst();

    void HandleOrbiterEscapingHost(NodeRef& node);
    void HandleOrbiterOverlappingInfluence(NodeRef& node);

    void RemoveOrbiterIntersectsFromSiblings(NodeRef& node, InflRef& parent);
    void ChangeNodeParent(NodeRef& node, InflRef& oldParent, InflRef& newParent);
    void RemoveNodeFromUpdateQueue(NodeRef& node);

    // debug resources
private:
    struct DebugData
    {
        std::shared_ptr<Limnova::CsvTable<float, uint32_t, float, float, float>> Table;
        std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<float, std::nano>> TFirstPeriapsePass;
        uint32_t NumPeriapsePasses = 0;
    };
    std::unordered_map<uint32_t, DebugData> m_DebugData;
    bool m_Testing = false;
    std::unordered_map<uint32_t, uint32_t> m_UpdateCounts;
private:
    void RecordData();
};
