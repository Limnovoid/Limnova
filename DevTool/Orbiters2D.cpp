#include "Orbiters2D.h"

#define ASSET_DIR "C:\\Programming\\source\\Limnova\\DevTool\\Assets"


static constexpr float kZoomMin = 0.05f;
static constexpr float kZoomMax = 1.5f;
static constexpr float kZoomDef = 1.f;
static constexpr float kZoomSen = 0.01f;


Orbiters2D::Orbiters2D()
    : Layer("Orbiters2D")
{
}


Orbiters2D::~Orbiters2D()
{
}


void Orbiters2D::OnAttach()
{
    LV::Application& app = LV::Application::Get();
    m_CameraController = std::make_shared<LV::OrthographicPlanarCameraController>(
        LV::Vector3(0.f, 0.f, 2.f), LV::Vector3(0.f, 0.f, -1.f),
        (float)app.GetWindow().GetWidth() / (float)app.GetWindow().GetHeight(), 0.1f, 100.f
    );
    m_CameraController->SetControlled(true);
    m_CameraController->SetZoomLimits(kZoomMin, kZoomMax);
    m_CameraController->SetZoomSensitivity(kZoomSen);

    // OrbitSystem
    OrbitSystem2D::Init();
    OrbitSystem2D& orbs = OrbitSystem2D::Get();
    m_Timescale = 0.01;
    orbs.SetTimeScale(m_Timescale);

    orbs.SetOrbiterChangedHostCallback([&](const uint32_t id, const bool escaped)
        {
            if (m_CameraTrackingId == id)
            {
                m_CameraController->SetXY({ 0.f,0.f });

                // Adjust zoom for easiest-to-understand visual transition between orbit spaces
                if (escaped)
                {
                    // If 'zoomed' out to a higher orbital system such that the camera is not tracking the tracked orbiter itself,
                    // decrement the camera's relative level such that the camera is not zoomed out to an even higher system when
                    // the orbiter moves up a level. E.g, with a relative level of 2, the camera would be tracking the tracked
                    // orbiter's host; if the tracked orbiter escaped, the relative level would decrement to 1 and the camera
                    // would stay in the same system but would now be tracking the tracked orbiter which has entered this system.
                    if (m_CameraRelativeLevel > 1)
                    {
                        m_CameraRelativeLevel--;
                    }
                    else
                    {
                        // Tracked orbiter escaped old host - set closest zoom
                        m_CameraController->SetZoom(kZoomMin);
                    }
                }
                else
                {
                    // If 'zoomed' out to a higher orbital system such that the camera is not tracking the tracked orbiter itself,
                    // decrement the camera's relative level such that the camera is not zoomed in to a lower system when the
                    // orbiter moves down a level.
                    if (m_CameraRelativeLevel > 1)
                    {
                        m_CameraRelativeLevel++;

                        // TODO - signal the player that the ship has entered a new influence (e.g. alert sound + message)
                    }
                    else
                    {
                        // Orbiter overlapped new host - set zoom to fit entire orbit space of new host
                        m_CameraController->SetZoom(kZoomDef);
                    }
                }
            }
        });

    orbs.SetOrbiterDestroyedCallback([&](const uint32_t id)
        {
            // If camera is tracking the destroyed orbiter, switch tracking to the orbiter's host and reset camera zoom
            if (id == m_CameraTrackingId)
            {
                m_CameraTrackingId = orbs.GetOrbiterHost(id);
                m_CameraRelativeLevel = 0;
                m_CameraController->SetXY({ 0.f,0.f });
                m_CameraController->SetZoom(kZoomDef);
            }

            // Delete information
            m_Orbiters[id]->Destroy();
            m_Orbiters.erase(id);
        });

    m_SystemHost = SystemHost::Create("Star", 0.05f, { 0.9f, 1.f, 1.f, 1.f }, { 1.498284464f, 10 }, { 1.f, 0 });
    m_Orbiters[m_SystemHost->GetOrbitSystemId()] = m_SystemHost;
    m_CameraTrackingId = m_SystemHost->GetOrbitSystemId();

    InflOrbRef planet0 = InfluencingOrbiter::Create("Planet 0", 0.001f, { 0.3f, 0.5f, 1.f, 1.f },
        LV::BigFloat(2.f, 6), m_SystemHost, LV::Vector2(1.f, 0.f), LV::BigVector2(0.f, 0.8f));
    m_Orbiters[planet0->GetOrbitSystemId()] = planet0;
    std::ostringstream nameoss;
    nameoss.str(""); nameoss << "Planet 0 (" << planet0->GetOrbitSystemId() << ")";
    planet0->SetName(nameoss.str());
    orbs.SetOrbiterRightAscension(planet0->GetOrbitSystemId(), LV::PIover4f);
    //m_CameraTrackingId = id; // track Planet 0
    {
        InflOrbRef moon0_0 = InfluencingOrbiter::Create("Moon 0.0", 0.00005f, { 0.3f, 0.9f, 1.f, 1.f },
            LV::BigFloat(1.f, 2), planet0, LV::Vector2(0.f, 0.9f), false);
        nameoss.str(""); nameoss << "Moon 0.0 (" << moon0_0->GetOrbitSystemId() << ")";
        moon0_0->SetName(nameoss.str());
        m_Orbiters[moon0_0->GetOrbitSystemId()] = moon0_0;

        InflOrbRef moon0_1 = InfluencingOrbiter::Create("Moon 0.1", 0.00005f, { 0.3f, 0.9f, 1.f, 1.f },
            LV::BigFloat(1.5f, 2), planet0, LV::BigVector2(-0.3f, 0.f), true);
        nameoss.str(""); nameoss << "Moon 0.1 (" << moon0_1->GetOrbitSystemId() << ")";
        moon0_1->SetName(nameoss.str());
        m_Orbiters[moon0_1->GetOrbitSystemId()] = moon0_1;

        // Testing dynamic orbits - orbiter self-acceleration
        m_PlayerShip = Spacecraft::Create("Player Ship", 0.00003f, { 0.4f, 0.4f, 0.45f, 1.f },
            LV::BigFloat(1.f, -1.f), planet0, LV::Vector2(0.15f, 0.f), LV::Vector2(0.f, 13.1f));
        nameoss.str(""); nameoss << "Player Ship (" << m_PlayerShip->GetOrbitSystemId() << ")";
        m_PlayerShip->SetName(nameoss.str());
        m_Orbiters[m_PlayerShip->GetOrbitSystemId()] = m_PlayerShip;

        m_CameraTrackingId = m_PlayerShip->GetOrbitSystemId(); // Track Player Ship

        //id = orbs.CreateOrbiterCS(false, true, LV::BigFloat(1.f, 2.f), planet0Id, LV::Vector2(-0.7f, 0.f), false);
        //nameoss.str(""); nameoss << "Practice Target 0 (" << id << ")";
        //m_OrbiterRenderInfo[id] = { nameoss.str(), 0.00003f, {0.7f, 0.4f, 0.3f, 1.f} };
    }

    InflOrbRef planet1 = InfluencingOrbiter::Create("Planet 1", 0.001f, { 0.2f, 0.7f, 1.f, 1.f },
        LV::BigFloat(1.f, 6), m_SystemHost, LV::Vector2(0.f, -.5f), false);
    nameoss.str(""); nameoss << "Planet 1 (" << planet1->GetOrbitSystemId() << ")";
    planet1->SetName(nameoss.str());
    m_Orbiters[planet1->GetOrbitSystemId()] = planet1;
    {
        InflOrbRef moon1_0 = InfluencingOrbiter::Create("Moon 1.0", 0.00003f, { 0.5f, 0.2f, .3f, 1.f },
            LV::BigFloat(1.f, 2), planet1, LV::Vector2(0.f, -.7f), false);
        nameoss.str(""); nameoss << "Moon 1.0 (" << moon1_0->GetOrbitSystemId() << ")";
        moon1_0->SetName(nameoss.str());
        m_Orbiters[moon1_0->GetOrbitSystemId()] = moon1_0;
    }

    // Textures
    m_CheckerboardTexture = LV::Texture2D::Create(ASSET_DIR"\\textures\\testtex.png", LV::Texture::WrapMode::MirroredTile);
    m_CircleFillTexture = LV::Texture2D::Create(ASSET_DIR"\\textures\\orbiter-0.png", LV::Texture::WrapMode::Clamp);
    m_CircleTexture = LV::Texture2D::Create(ASSET_DIR"\\textures\\orbit-a1270.png", LV::Texture::WrapMode::Clamp);
    m_CircleThickTexture = LV::Texture2D::Create(ASSET_DIR"\\textures\\circleThick.png", LV::Texture::WrapMode::Clamp);
    m_CircleLargeFillTexture = LV::Texture2D::Create(ASSET_DIR"\\textures\\circleFill_d1270.png", LV::Texture::WrapMode::Clamp);
}


void Orbiters2D::OnDetach()
{
    OrbitSystem2D::Get().Shutdown();
}


void Orbiters2D::OnUpdate(LV::Timestep dT)
{
    LV_PROFILE_FUNCTION();

    OrbitSystem2D& orbs = OrbitSystem2D::Get();

    float zoom;
    float orbiterCircleRadius;

    // Map the scene - get IDs of orbiters in the scene, update their Entity positions (transforms)
    // with their positions relative to the tracked orbiter.
    uint32_t sceneHostId, sceneTrackingId;
    bool cameraIsTrackingHost;

    std::vector<uint32_t> sceneOrbiters;
    std::unordered_map<uint32_t, OrbiterUI> sceneOrbiterUi;

    LV::Vector2 shipPos, shipToMouseLine;

    // Update
    {
        LV_PROFILE_SCOPE("Update - Dev2DLayer::OnUpdate");

        static constexpr float shipAcceleration = 0.5f;
        static constexpr float baseOrbiterCircleRadius = 0.024f;
        static constexpr float weaponMuzzleVelocity = 5.f;

        zoom = m_CameraController->GetZoom();
        orbiterCircleRadius = zoom * baseOrbiterCircleRadius;

        // Player input data
        float mouseX, mouseY;
        std::tie(mouseX, mouseY) = LV::Input::GetMousePosition();
        m_Input.MouseScenePosition = m_CameraController->GetWorldXY({ mouseX, mouseY });

        // Map the scene - get IDs of orbiters in the scene, update their Entity positions (transforms)
        // with their positions relative to the tracked orbiter.
        GetCameraTrackingIds(&sceneHostId, &sceneTrackingId);
        cameraIsTrackingHost = sceneTrackingId == sceneHostId;

        LV::Vector2 sceneHostPos = cameraIsTrackingHost ? 0.f : -1.f * m_Orbiters[sceneTrackingId]->GetParameters().Position;
        m_Orbiters[sceneHostId]->SetPosition({ sceneHostPos, 0.f });

        orbs.GetOrbiters(sceneHostId, sceneOrbiters);
        for (uint32_t id : sceneOrbiters)
        {
            auto& orbRef = m_Orbiters[id];

            LV::Vector2 sceneOrbPos{ 0.f, 0.f };
            if (id != sceneTrackingId)
            {
                sceneOrbPos = sceneHostPos + orbRef->GetParameters().Position;
            }
            orbRef->SetPosition({ sceneOrbPos, 0.f });

            sceneOrbiterUi[id].IsHovered = sqrt((m_Input.MouseScenePosition - orbRef->GetPosition().XY()).SqrMagnitude()) < orbiterCircleRadius;
            if (sceneOrbiterUi[id].IsHovered && LV::Input::IsMouseButtonPressed(LV_MOUSE_BUTTON_LEFT))
            {
                m_CameraTrackingId = id;
                m_CameraRelativeLevel = 1;
            }

            if (orbs.IsInfluencing(id) && id == sceneTrackingId)
            {
                float roi = orbs.GetRadiusOfInfluence(id);
                orbs.GetOrbiters(id, sceneOrbiterUi[id].SubOrbiters);
                for (uint32_t sid : sceneOrbiterUi[id].SubOrbiters)
                {
                    LV::Vector2 subPos = m_Orbiters[sid]->GetParameters().Position * roi;
                    m_Orbiters[sid]->SetPosition({ subPos, 0.f });
                }
            }
        }

        // Ship is controlled if the player ship is visible
        m_Input.ShipIsBeingControlled = PlayerShipIsVisible(sceneHostId, sceneTrackingId);
        if (m_Input.ShipIsBeingControlled)
        {
            // Get line from Player Ship to mouse position
            if (sceneTrackingId == m_PlayerShip->GetHostOrbitSystemId())
            {
                float posScaling = sceneHostId == sceneTrackingId ? 1.f : orbs.GetRadiusOfInfluence(sceneTrackingId);
                shipPos = posScaling * m_PlayerShip->GetParameters().Position;
            }
            else if (sceneTrackingId != m_PlayerShip->GetOrbitSystemId())
            {
                shipPos = m_PlayerShip->GetParameters().Position - m_Orbiters[sceneTrackingId]->GetParameters().Position;
            }
            shipToMouseLine = m_Input.MouseScenePosition - shipPos;
            if (shipToMouseLine.SqrMagnitude() > 0)
            {
                if (m_Input.WeaponSelected)
                {
                    LV::BigVector2 weaponLaunchVelocity = weaponMuzzleVelocity * shipToMouseLine.Normalized();
                    m_Input.TargetingOrbit = orbs.ComputeOrbit(m_PlayerShip->GetHostOrbitSystemId(),
                        m_PlayerShip->GetParameters().Position, m_PlayerShip->GetParameters().Velocity + weaponLaunchVelocity);
                }
                else if (LV::Input::IsMouseButtonPressed(LV_MOUSE_BUTTON_LEFT))
                {
                    m_Input.ShipIsThrusting = true;
                    m_PlayerShip->Accelerate(LV::BigVector2{ shipAcceleration * shipToMouseLine.Normalized() });
                }
            }
            else
            {
                m_Input.ShipIsThrusting = false;
            }
        }

        // Orbit system
        orbs.Update(dT);

        m_CameraController->OnUpdate(dT);

        // Check if camera is zooming in or out of the current scene sytem
        if (m_ZoomingIntoSystem && m_CameraRelativeLevel > 0)
        {
            m_CameraRelativeLevel--;
            m_CameraController->SetXY({ 0.f, 0.f });
            m_CameraController->SetZoom(kZoomMax);
            m_ZoomingIntoSystem = false;
        }
        if (m_ZoomingOutOfSystem && sceneHostId != m_SystemHost->GetOrbitSystemId())
        {
            m_CameraRelativeLevel++;
            m_CameraController->SetXY({ 0.f, 0.f });
            m_CameraController->SetZoom(kZoomMin);
            m_ZoomingOutOfSystem = false;
        }
    }

    // Render
    {
        LV_PROFILE_SCOPE("Render Prep - Orbiters2D::OnUpdate");

        LV::RenderCommand::SetClearColor({ 0.f, 0.f, 0.f, 1.f });
        LV::RenderCommand::Clear();
    }

    {
        LV_PROFILE_SCOPE("Render Draw - Orbiters2D::OnUpdate");

        LV::Renderer2D::BeginScene(m_CameraController->GetCamera());

        static constexpr float circleFillTexSizeFactor = 4.f; // Texture widths per unit circle-RADII
        static constexpr float circleTexSizeFactor = 2.f * 1280.f / 1270.f; // Texture widths per unit circle-DIAMETERS
        static constexpr float circleThickTexSizeFactor = 2.f * 128.f / 110.f;
        static constexpr float baseTrajectoryLineThickness = 0.008f;
        static constexpr float subTrajectoryLineThickness = kZoomMin * baseTrajectoryLineThickness;
        static constexpr float baseIntersectCircleRadius = 0.016f;
        static constexpr float trackedSubOrbiterRadius = 0.001f;
        static constexpr float baseShipThrustLineThickness = 0.008f;
        static constexpr float circleLargeFillTexSizeFactor = 1280.f / 1270.f; // Texture widths per unit circle-DIAMETERS
        static const LV::Vector4 weaponCol{ 0.5f, 1.f, 1.f, 0.3f };

        float trajectoryLineThickness = zoom * baseTrajectoryLineThickness;
        float intersectCircleRadius = zoom * baseIntersectCircleRadius;
        float shipThrustLineThickness = zoom * baseShipThrustLineThickness;

        float sceneScaling = orbs.GetScaling(sceneHostId);

        auto& hostRef = m_Orbiters[sceneHostId];
        float hostQuadWidth = circleLargeFillTexSizeFactor * 2.f * hostRef->GetRadius() * sceneScaling;
        LV::Renderer2D::DrawQuad(hostRef->GetPosition(), {hostQuadWidth, hostQuadWidth}, m_CircleLargeFillTexture, hostRef->GetColor());

        // Render all visible orbiters and their influences
        for (uint32_t id : sceneOrbiters)
        {
            auto& orbRef = m_Orbiters[id];
            bool cameraIsTrackingOrbiter = id == sceneTrackingId;

            // Orbiter
            LV::Renderer2D::DrawQuad(orbRef->GetPosition(), {circleFillTexSizeFactor * orbRef->GetRadius() * sceneScaling}, m_CircleFillTexture, orbRef->GetColor());

            // UI orbiter icon
            // TEMPORARY - UI elements should be separate ?
            auto iconCol = orbRef->GetColor();
            iconCol.x = powf(iconCol.x + 0.1f, 2);
            iconCol.y = powf(iconCol.y + 0.1f, 2);
            iconCol.z = powf(iconCol.z + 0.1f, 2);
            iconCol.w = (cameraIsTrackingOrbiter || sceneOrbiterUi[id].IsHovered) ? 0.7f : 0.3f;
            LV::Renderer2D::DrawQuad(orbRef->GetPosition().XY(), {circleThickTexSizeFactor * orbiterCircleRadius}, m_CircleThickTexture, iconCol);

            // Influence
            float roi = orbs.GetRadiusOfInfluence(id);
            if (orbs.IsInfluencing(id))
            {
                float roiQuadDiameter = circleLargeFillTexSizeFactor * 2.f * roi;
                LV::Renderer2D::DrawQuad(orbRef->GetPosition(), { roiQuadDiameter }, m_CircleLargeFillTexture, m_InfluenceColor);
            }

            // Suborbiters
            for (uint32_t sid : sceneOrbiterUi[id].SubOrbiters)
            {
                auto col = m_Orbiters[sid]->GetColor();
                col.w = sid == m_CameraTrackingId ? 0.7f : 0.3f;
                LV::Renderer2D::DrawQuad(m_Orbiters[sid]->GetPosition().XY(), { circleFillTexSizeFactor * trackedSubOrbiterRadius}, m_CircleFillTexture, col);
            }
        }

        // Draw line from Player Ship to mouse position
        if (m_Input.ShipIsBeingControlled)
        {
            auto shipInputUICol = m_PlayerShip->GetColor();
            shipInputUICol.x = powf(shipInputUICol.x + 0.1f, 2);
            shipInputUICol.y = powf(shipInputUICol.y + 0.1f, 2);
            shipInputUICol.z = powf(shipInputUICol.z + 0.1f, 2);
            shipInputUICol.w = m_Input.ShipIsThrusting ? 0.7f : 0.3f;
            LV::Renderer2D::DrawLine(shipPos, shipPos + shipToMouseLine, shipThrustLineThickness, shipInputUICol);
        }

        // Render linear trajectories
        for (uint32_t id : sceneOrbiters)
        {
            bool cameraIsTrackingOrbiter = id == sceneTrackingId;

            auto& orbRef = m_Orbiters[id];

            if (orbRef->GetParameters().NewtonianMotion)
            {
                auto col = orbRef->GetColor();
                col.w = cameraIsTrackingOrbiter ? 0.7f : 0.3f;
                LV::Renderer2D::DrawLine(hostRef->GetPosition().XY(), orbRef->GetPosition().XY(), trajectoryLineThickness, col);
            }

            // Sub-orbiters
            for (uint32_t sid : sceneOrbiterUi[id].SubOrbiters)
            {
                auto& subOrbRef = m_Orbiters[sid];
                if (subOrbRef->GetParameters().NewtonianMotion)
                {
                    auto col = subOrbRef->GetColor();
                    col.w = sid == m_CameraTrackingId ? 0.7f : 0.3f;
                    LV::Renderer2D::DrawLine(orbRef->GetPosition().XY(), subOrbRef->GetPosition().XY(), subTrajectoryLineThickness, col);
                }
            }

        }
        // Linear targeting orbit
        if (m_Input.TargetingOrbit.NewtonianMotion && m_CameraRelativeLevel > 0 && m_Input.ShipIsBeingControlled && m_Input.WeaponSelected)
        {
            float scale = m_CameraRelativeLevel == 1 ? 1.f : orbs.GetScaling(m_PlayerShip->GetHostOrbitSystemId());
            LV::Renderer2D::DrawLine(m_Orbiters[m_PlayerShip->GetHostOrbitSystemId()]->GetPosition().XY(),
                m_PlayerShip->GetPosition().XY(), m_CameraRelativeLevel == 1 ? trajectoryLineThickness : subTrajectoryLineThickness, weaponCol);
        }

        // Render elliptical orbits/trajectories
        LV::Renderer2D::TEMP_BeginEllipses(); // TEMPORARY: separate draw calls for different shaders - TODO: use render queue
        for (uint32_t id : sceneOrbiters)
        {
            bool cameraIsTrackingOrbiter = id == sceneTrackingId;

            auto& orbRef = m_Orbiters[id];
            auto& op = orbRef->GetParameters();

            if (op.Type == OrbitSystem2D::OrbitType::Circle || op.Type == OrbitSystem2D::OrbitType::Ellipse)
            {
                LV::Vector2 centrePos = hostRef->GetPosition().XY() + op.Centre;
                LV::Vector2 escapePointFromCentre{ 0.f, 0.f };
                if (op.TrueAnomalyEscape < LV::PI2f)
                {
                    float distanceCentreFocus = op.Eccentricity * op.SemiMajorAxis;
                    escapePointFromCentre = { distanceCentreFocus + op.EscapePointPerifocal.x, op.EscapePointPerifocal.y };
                }
                auto col = orbRef->GetColor();
                col.w = cameraIsTrackingOrbiter ? 0.7f : 0.3f;
                LV::Renderer2D::DrawEllipse(centrePos, op.RightAscensionPeriapsis, op.SemiMajorAxis, op.SemiMinorAxis,
                    escapePointFromCentre, trajectoryLineThickness, col);
            }

            // Sub-orbiters
            float roi = orbs.GetRadiusOfInfluence(id);
            for (uint32_t sid : sceneOrbiterUi[id].SubOrbiters)
            {
                auto& subOrbRef = m_Orbiters[sid];
                auto& sop = subOrbRef->GetParameters();
                if (sop.Type == OrbitSystem2D::OrbitType::Circle || sop.Type == OrbitSystem2D::OrbitType::Ellipse)
                {
                    LV::Vector2 centrePos = orbRef->GetPosition().XY() + roi * sop.Centre;
                    LV::Vector2 escapePointFromCentre{ 0.f, 0.f };
                    if (sop.TrueAnomalyEscape < LV::PI2f)
                    {
                        float distanceCentreFocus = sop.Eccentricity * sop.SemiMajorAxis;
                        escapePointFromCentre = { distanceCentreFocus + sop.EscapePointPerifocal.x, sop.EscapePointPerifocal.y };
                    }
                    auto col = subOrbRef->GetColor();
                    col.w = sid == m_CameraTrackingId ? 0.7f : 0.3f;
                    LV::Renderer2D::DrawEllipse(centrePos, sop.RightAscensionPeriapsis, roi * sop.SemiMajorAxis, roi * sop.SemiMinorAxis,
                        roi * escapePointFromCentre, subTrajectoryLineThickness, col);
                }
            }
        }
        // Elliptical targeting orbit
        if ((m_Input.TargetingOrbit.Type == OrbitSystem2D::OrbitType::Circle || m_Input.TargetingOrbit.Type == OrbitSystem2D::OrbitType::Ellipse)
            && m_CameraRelativeLevel > 0 && m_Input.ShipIsBeingControlled && m_Input.WeaponSelected)
        {
            float scale = m_CameraRelativeLevel == 1 ? 1.f : orbs.GetScaling(m_PlayerShip->GetHostOrbitSystemId());
            LV::Vector2 centrePos = m_Orbiters[m_PlayerShip->GetHostOrbitSystemId()]->GetPosition().XY() + scale * m_Input.TargetingOrbit.Centre;
            LV::Vector2 escapePointFromCentre{ 0.f, 0.f };
            if (m_Input.TargetingOrbit.TrueAnomalyEscape < LV::PI2f)
            {
                float distanceCentreFocus = m_Input.TargetingOrbit.Eccentricity * m_Input.TargetingOrbit.SemiMajorAxis;
                escapePointFromCentre = { distanceCentreFocus + m_Input.TargetingOrbit.EscapePointPerifocal.x, m_Input.TargetingOrbit.EscapePointPerifocal.y };
            }
            LV::Renderer2D::DrawEllipse(centrePos, m_Input.TargetingOrbit.RightAscensionPeriapsis,
                m_Input.TargetingOrbit.SemiMajorAxis * scale, m_Input.TargetingOrbit.SemiMinorAxis * scale,
                escapePointFromCentre * scale, m_CameraRelativeLevel == 1 ? trajectoryLineThickness : subTrajectoryLineThickness, weaponCol);
        }

        // Render hyperbolic trajectories
        LV::Renderer2D::TEMP_BeginHyperbolae(); // TEMPORARY: separate draw calls for different shaders - TODO: use render queue
        for (uint32_t id : sceneOrbiters)
        {
            bool cameraIsTrackingOrbiter = id == sceneTrackingId;

            auto& orbRef = m_Orbiters[id];
            auto& op = orbRef->GetParameters();

            if (op.Type == OrbitSystem2D::OrbitType::Hyperbola)
            {
                LV::Vector2 centrePos = orbRef->GetPosition().XY() + op.Centre;

                float distanceCentreFocus = op.Eccentricity * op.SemiMajorAxis;
                LV::Vector2 escapePointFromCentre{ distanceCentreFocus - op.EscapePointPerifocal.x, op.EscapePointPerifocal.y };

                auto col = orbRef->GetColor();
                col.w = cameraIsTrackingOrbiter ? 0.7f : 0.3f;
                LV::Renderer2D::DrawHyperbola(centrePos, op.RightAscensionPeriapsis, op.SemiMajorAxis, op.SemiMinorAxis,
                    escapePointFromCentre, trajectoryLineThickness, col);
            }

            // Sub-orbiters
            float roi = orbs.GetRadiusOfInfluence(id);
            for (uint32_t sid : sceneOrbiterUi[id].SubOrbiters)
            {
                auto& subOrbRef = m_Orbiters[sid];
                auto& sop = subOrbRef->GetParameters();
                if (sop.Type == OrbitSystem2D::OrbitType::Hyperbola)
                {
                    LV::Vector2 centrePos = orbRef->GetPosition().XY() + roi * sop.Centre;

                    float distanceCentreFocus = op.Eccentricity * op.SemiMajorAxis;
                    LV::Vector2 escapePointFromCentre{ distanceCentreFocus - op.EscapePointPerifocal.x, op.EscapePointPerifocal.y };

                    auto col = subOrbRef->GetColor();
                    col.w = sid == m_CameraTrackingId ? 0.7f : 0.3f;
                    LV::Renderer2D::DrawHyperbola(centrePos, sop.RightAscensionPeriapsis, roi * sop.SemiMajorAxis, roi * sop.SemiMinorAxis,
                        roi * escapePointFromCentre, subTrajectoryLineThickness, col);
                }
            }
        }
        // Hyperbolic targeting orbit
        if (m_Input.TargetingOrbit.Type == OrbitSystem2D::OrbitType::Hyperbola
            && m_CameraRelativeLevel > 0 && m_Input.ShipIsBeingControlled && m_Input.WeaponSelected)
        {
            float scale = m_CameraRelativeLevel == 1 ? 1.f : orbs.GetScaling(m_PlayerShip->GetHostOrbitSystemId());
            LV::Vector2 centrePos = m_Orbiters[m_PlayerShip->GetHostOrbitSystemId()]->GetPosition().XY() + scale * m_Input.TargetingOrbit.Centre;
            float distanceCentreFocus = m_Input.TargetingOrbit.Eccentricity * m_Input.TargetingOrbit.SemiMajorAxis;
            LV::Vector2 escapePointFromCentre{ distanceCentreFocus - m_Input.TargetingOrbit.EscapePointPerifocal.x, m_Input.TargetingOrbit.EscapePointPerifocal.y };
            LV::Renderer2D::DrawHyperbola(centrePos, m_Input.TargetingOrbit.RightAscensionPeriapsis,
                m_Input.TargetingOrbit.SemiMajorAxis * scale, m_Input.TargetingOrbit.SemiMinorAxis * scale,
                escapePointFromCentre * scale, m_CameraRelativeLevel == 1 ? trajectoryLineThickness : subTrajectoryLineThickness, weaponCol);
        }

        LV::Renderer2D::EndScene();
    }
}


void Orbiters2D::OnImGuiRender()
{
    ImGui::Begin("Orbiters2D");

    OrbitSystem2D& orbs = OrbitSystem2D::Get();

    if (ImGui::SliderFloat("Timescale", &m_Timescale, 0.f, 1.f))
    {
        orbs.SetTimeScale(m_Timescale);
    }

    // Get IDs of scene host and camera-tracked orbiter
    uint32_t sceneHostId, sceneTrackingId;
    GetCameraTrackingIds(&sceneHostId, &sceneTrackingId);
    bool cameraIsTrackingHost = sceneTrackingId == sceneHostId;

    // Orbiter HUD colors
    std::vector<uint32_t> trackableOrbiterIds;
    trackableOrbiterIds.push_back(sceneHostId);
    orbs.GetOrbiters(sceneHostId, trackableOrbiterIds);
    for (uint32_t idx = 1; idx < trackableOrbiterIds.size(); idx++)
    {
        auto col = m_Orbiters[trackableOrbiterIds[idx]]->GetColor();
        ImGui::ColorEdit4(m_Orbiters[trackableOrbiterIds[idx]]->GetName().c_str(),
            glm::value_ptr(*(glm::vec4*)&col));
        m_Orbiters[trackableOrbiterIds[idx]]->SetColor(col);
    }

    // Camera tracking orbiter selection
    //if (ImGui::BeginCombo("Orbiter Tracking", m_Orbiters[sceneTrackingId]->GetName().c_str()))
    //{
    //    for (uint32_t id : trackableOrbiterIds)
    //    {
    //        const bool isSelected = (m_CameraTrackingId == id);
    //        if (ImGui::Selectable(m_Orbiters[id]->GetName().c_str(), isSelected))
    //        {
    //            m_CameraTrackingId = id;
    //            m_CameraController->SetXY({ 0.f, 0.f });
    //        }

    //        // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
    //        if (isSelected)
    //            ImGui::SetItemDefaultFocus();
    //    }
    //    ImGui::EndCombo();
    //}

    // Orbiter information table
    ImGui::BeginTable("Orbiter Information", 5, ImGuiTableFlags_Borders);
    // Headers
    ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("Orbiter");
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("True Anomaly");
    ImGui::TableSetColumnIndex(2);
    ImGui::Text("Speed");
    ImGui::TableSetColumnIndex(3);
    ImGui::Text("ROI");
    ImGui::TableSetColumnIndex(4);
    ImGui::Text("Semi-major Axis");
    // Information - host
    auto& host = orbs.GetHost(sceneHostId);
    float scaling = 0 == sceneHostId ? 1.f : host.GetHostScaling();
    auto& op = host.GetParameters();
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text(m_Orbiters[sceneHostId]->GetName().c_str());
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%.2f (%.2f)", op.TrueAnomaly, glm::degrees(op.TrueAnomaly));
    ImGui::TableSetColumnIndex(2);
    ImGui::Text("%.2f (%.4f)", sqrtf(op.Velocity.SqrMagnitude().Float()), sqrtf(op.Velocity.SqrMagnitude().Float()) / scaling);
    ImGui::TableSetColumnIndex(3);
    float roi = host.GetRadiusOfInfluence();
    ImGui::Text("%.4f (%.6f)", roi, roi / scaling);
    ImGui::TableSetColumnIndex(4);
    ImGui::Text("%.4f (%.6f)", op.SemiMajorAxis, op.SemiMajorAxis / scaling);
    // Information - orbiters
    scaling = host.GetScaling();
    for (uint32_t idx = 1; idx < trackableOrbiterIds.size(); idx++)
    {
        auto id = trackableOrbiterIds[idx];
        auto& op = orbs.GetParameters(id);
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text(m_Orbiters[id]->GetName().c_str());
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%.2f (%.2f)", op.TrueAnomaly, glm::degrees(op.TrueAnomaly));
        ImGui::TableSetColumnIndex(2);
        ImGui::Text("%.2f (%.4f)", sqrtf(op.Velocity.SqrMagnitude().Float()), sqrtf(op.Velocity.SqrMagnitude().Float()) / scaling);
        ImGui::TableSetColumnIndex(3);
        float roi = orbs.IsInfluencing(id) ? orbs.GetRadiusOfInfluence(id) : 0.f;
        ImGui::Text("%.4f (%.6f)", roi, roi / scaling);
        ImGui::TableSetColumnIndex(4);
        ImGui::Text("%.4f (%.6f)", op.SemiMajorAxis, op.SemiMajorAxis / scaling);
    }

    ImGui::EndTable();

    ImGui::End();
}


void Orbiters2D::OnEvent(LV::Event& e)
{
    LV::EventDispatcher dispatcher(e);
    dispatcher.Dispatch<LV::MouseScrolledEvent>(LV_BIND_EVENT_FN(Orbiters2D::OnMouseScrolled));
    dispatcher.Dispatch<LV::MouseButtonPressedEvent>(LV_BIND_EVENT_FN(Orbiters2D::OnMouseButtonPressed));
    dispatcher.Dispatch<LV::KeyPressedEvent>(LV_BIND_EVENT_FN(Orbiters2D::OnKeyPressed));

    m_CameraController->OnEvent(e);
}


void Orbiters2D::GetCameraTrackingIds(uint32_t* sceneHostId, uint32_t* sceneTrackingId)
{
    auto& orbs = OrbitSystem2D::Get();
    *sceneHostId = m_CameraTrackingId;
    *sceneTrackingId = m_CameraTrackingId;
    for (uint32_t l = 0; l < m_CameraRelativeLevel; l++)
    {
        *sceneTrackingId = *sceneHostId;
        *sceneHostId = orbs.GetHostId(*sceneHostId);
    }
}


bool Orbiters2D::PlayerShipIsVisible(const uint32_t sceneHostId, const uint32_t sceneTrackingId)
{
    return (m_CameraTrackingId == m_PlayerShip->GetOrbitSystemId() && m_CameraRelativeLevel < 2)
        || sceneTrackingId == m_PlayerShip->GetHostOrbitSystemId();
}


bool Orbiters2D::OnMouseButtonPressed(LV::MouseButtonPressedEvent& e)
{
    switch (e.GetMouseButton())
    {
    case LV_MOUSE_BUTTON_LEFT:
        if (m_Input.ShipIsBeingControlled && m_Input.WeaponSelected)
        {
            // Fire weapon
        }
        break;
    }
    return true;
}


bool Orbiters2D::OnMouseScrolled(LV::MouseScrolledEvent& e)
{
    m_ZoomingOutOfSystem = e.GetYOffset() < 0 && m_CameraController->GetZoom() == kZoomMax;
    m_ZoomingIntoSystem = e.GetYOffset() > 0 && m_CameraController->GetZoom() == kZoomMin;

    return false;
}


bool Orbiters2D::OnKeyPressed(LV::KeyPressedEvent& e)
{
    switch (e.GetKeyCode())
    {
    case LV_KEY_1:
        m_Input.WeaponSelected = !m_Input.WeaponSelected;
        break;
    }
    return true;
}


// Projectile ////

//const LV::Vector4 Orbiters2D::Projectile::s_Color = { 1.f, 1.f, 0.5f, 1.f };
//const LV::BigFloat Orbiters2D::Projectile::s_Mass = { 1.f, -4 };
//
//Orbiters2D::Projectile::Projectile(const OrbRef& launcher, const InflOrbRef& launcherHost, const OrbRef& target, const LV::Vector2& scaledLaunchVelocity)
//{
//    m_OrbRef = Orbiter::Create("Projectile", s_Radius, s_Color, s_Mass, launcherHost, launcher->GetParameters().Position, launcher->GetParameters().Velocity + scaledLaunchVelocity);
//}
