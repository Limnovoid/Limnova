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
        LVM::BigFloat Gravitational = { 0.f, 0 };

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
        float CcwF = 1; // Counter-clockwise factor

        // Computation constants
        float mu;
        float h2mu;
        float muh;
    };
public:
    static void Init();
    static OrbitSystem2D& Get();

    void Update(Limnova::Timestep dT);

    void LoadLevel(const LVM::BigFloat& hostMass);
    // CreateOrbiter returns ID of created orbiter - use ID to get render info with GetParameters.
    uint32_t CreateOrbiter(const LVM::BigFloat& mass, const Limnova::Vector2& position, const Limnova::Vector2& velocity);
    uint32_t CreateOrbiter(const LVM::BigFloat& mass, const Limnova::Vector2& position, bool clockwise = false);
    //uint32_t CreateOrbiter(const LVM::BigFloat& mass, const Limnova::Vector2& position, float eccentricity, float trueAnomaly = 0.f, bool clockwise = false);


    const OrbitParameters& GetParameters(const uint32_t orbiter);
    const OrbitParameters& GetHostRenderInfo();

    void SetOrbiterRightAscension(const uint32_t orbiter, const float rightAscension);
private:
    struct OrbitTreeNode
    {
        std::shared_ptr<OrbitTreeNode> Parent;
        std::vector<std::shared_ptr<OrbitTreeNode>> Children;
        OrbitParameters Parameters;
        bool NeedRecomputeState = false;
    };

    std::shared_ptr<OrbitTreeNode> m_LevelHost;
    std::vector<std::shared_ptr<OrbitTreeNode>> m_Nodes;
private:
    static void ComputeElementsFromState(OrbitParameters& params);
    static void ComputeStateVector(OrbitParameters& params);
};
