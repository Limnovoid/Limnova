#pragma once

#include <Limnova.h>

#include "Orbiter.h"


class OrbitSystem2D
{
private:
    static OrbitSystem2D s_OrbitSystem2D;
public:
    struct OrbitParameters
    {
        Limnova::BigFloat GravAsHost = { 0.f, 0 }; // Gravitational parameter of this orbiter, used by its own children for computing their orbits around this host.
        Limnova::BigFloat GravAsOrbiter = { 0.f, 0 }; // Gravitation paramter of this orbiter's host, used for this orbiter's own computations.

        // State
        Limnova::Vector2 Position = { 0.f, 0.f };
        Limnova::Vector2 Velocity = { 0.f, 0.f };

        // Perifocal frame
        Limnova::Vector2 BasisX = { 1.f, 0.f };
        Limnova::Vector2 BasisY = { 0.f, 1.f };
        Limnova::Vector2 Centre = { 0.f, 0.f };
        float RightAscensionPeriapsis = 0.f;

        // Elements
        float OSAMomentum = 0.f; // Orbital specific angular momentum
        float Eccentricity = 0.f;
        float TrueAnomaly = 0.f;

        // Dimensions
        float SemiMajorAxis = 0.f;
        float SemiMinorAxis = 0.f;
        float Period = 0.f;
        float CcwF = 1.f; // Counter-clockwise factor

        // Computation constants
        float mu = 0.f; // TEMPORARY - realistic values are too large for floats!
        float h2mu = 0.f;
        float muh = 0.f;
    };
public:
    static void Init();
    static OrbitSystem2D& Get();

    void Update(Limnova::Timestep dT);

    void LoadLevel(const Limnova::BigFloat& hostMass, const Limnova::BigFloat& scaling);
    // CreateOrbiter returns ID of created orbiter - use ID to get render info with GetParameters. Returns ID of created orbiter, or numeric_limits<unit32_t>::max() if creation fails.
    uint32_t CreateOrbiter(const Limnova::BigFloat& mass, const Limnova::Vector2& position, const Limnova::Vector2& velocity);
    uint32_t CreateOrbiter(const Limnova::BigFloat& mass, const Limnova::Vector2& position, bool clockwise = false);
    //uint32_t CreateOrbiter(const Limnova::BigFloat& mass, const Limnova::Vector2& position, float eccentricity, float trueAnomaly = 0.f, bool clockwise = false);

    const OrbitParameters& GetParameters(const uint32_t orbiter);
    float GetRadiusOfInfluence(const uint32_t orbiter);
    float GetScaling(const uint32_t host);
    void GetChildren(const uint32_t host, std::vector<uint32_t>& ids);

    void GetAllHosts(std::vector<uint32_t>& ids);

    void SetOrbiterRightAscension(const uint32_t orbiter, const float rightAscension);

    void SetTimeScale(const float timescale) { m_Timescale = timescale; }

private:
    struct OrbitTreeNode;
    struct InfluencingNode;
    using NodeRef = std::shared_ptr<OrbitTreeNode>;
    using InflRef = std::shared_ptr<InfluencingNode>;

    struct OrbitTreeNode
    {
        InflRef Parent = nullptr;
        uint32_t Id = std::numeric_limits<uint32_t>::max();

        OrbitParameters Parameters;
        bool NeedRecomputeState = false;

        virtual ~OrbitTreeNode() {}
    };

    struct Influence
    {
        Limnova::BigFloat TotalScaling; // Use to multiply children's parameters to convert them from relative-scaled dimensions to unscaled-absolute dimensions.
        float Radius = 0.f; // Scaled by parent - use to multiply children's parameters to convert them from this influence's scale to the parent scale (move from this influence to the next-higher influence in the level).
    };

    struct InfluencingNode : public OrbitTreeNode
    {
        std::vector<InflRef> InfluencingChildren;
        std::vector<NodeRef> NonInflChildren; // Non-influencing children - low-mass orbiters, ships, etc

        Influence Influence;
    };
private:
    InflRef m_LevelHost;
    std::vector<InflRef> m_InflNodes;

    float m_Timescale = 1.f;
private:
    uint32_t CreateInfluencingNode(const InflRef& parent, const Limnova::BigFloat& mass, const Limnova::Vector2& scaledPosition, const Limnova::Vector2& scaledVelocity);
    static void ComputeElementsFromState(OrbitParameters& params);
    static void ComputeStateVector(OrbitParameters& params);
    static void ComputeInfluence(InfluencingNode* influencingNode, const InflRef& parent, const Limnova::BigFloat& mass);

    // Returns lowest-level node whose circle-of-influence overlaps the given position, or m_LevelHost if none overlaps.
    InflRef& FindLowestOverlappingInfluence(const Limnova::Vector2& absolutePosition);
    InflRef& FindOverlappingChildInfluence(InflRef& parent, const Limnova::Vector2& scaledPosition);
};
