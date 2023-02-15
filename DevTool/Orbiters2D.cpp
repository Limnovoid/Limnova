#include "Orbiters2D.h"

#define ASSET_DIR "C:\\Programming\\source\\Limnova\\DevTool\\Assets"


static constexpr float kZoomMin = 0.1f;
static constexpr float kZoomMax = 1.5f;
static constexpr float kZoomDef = 1.f;


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
    m_CameraController->SetZoomLimits(kZoomMin, kZoomMax);

    // OrbitSystem
    OrbitSystem2D::Init();
    OrbitSystem2D& orbs = OrbitSystem2D::Get();
    m_Timescale = 0.01;
    orbs.SetTimeScale(m_Timescale);

    orbs.SetOrbiterChangedHostCallback([&](const uint32_t id, const bool escaped)
        {
            if (m_CameraTrackingId == id &&
                m_CameraHostId != m_CameraTrackingId)
            {
                m_CameraHostId = orbs.GetOrbiterHost(id);
                m_CameraTrackingChanged = true; // Re-centre camera on tracked orbiter

                // Adjust zoom for easiest-to-understand visual transition between orbit spaces
                if (escaped)
                {
                    // Orbiter escaped old host - set closest zoom
                    m_CameraController->SetZoom(kZoomMin);
                }
                else
                {
                    // Orbiter overlapped new host - set zoom to fit entire orbit space of new host
                    m_CameraController->SetZoom(kZoomDef);
                }
            }
        });

    orbs.SetOrbiterDestroyedCallback([&](const uint32_t id)
        {
            // If camera is tracking the destroyed orbiter, switch tracking to the orbiter's host and reset camera zoom
            if (id == m_CameraTrackingId || id == m_CameraHostId)
            {
                m_CameraHostId = orbs.GetOrbiterHost(id);
                m_CameraTrackingId = m_CameraHostId;
                m_CameraTrackingChanged = true; // Re-centre camera on tracked orbiter
                m_CameraController->SetZoom(kZoomDef);
            }

            // Delete information
            m_Orbiters[id]->Destroy();
            m_Orbiters.erase(id);
        });

    m_SystemHost = SystemHost::Create("Star", 0.05f, { 0.9f, 1.f, 1.f, 1.f }, { 1.498284464f, 10 }, { 1.f, 0 });
    m_Orbiters[m_SystemHost->GetOrbitSystemId()] = m_SystemHost;
    m_CameraHostId = m_SystemHost->GetOrbitSystemId();
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
        m_PlayerShip = PlayerShip::Create("Player Ship", 0.00003f, { 0.6f, 0.6f, 0.4f, 1.f },
            LV::BigFloat(1.f, -1.f), planet0, LV::Vector2(0.15f, 0.f), LV::Vector2(0.f, 13.1f));
        nameoss.str(""); nameoss << "Player Ship (" << m_PlayerShip->GetOrbitSystemId() << ")";
        m_PlayerShip->SetName(nameoss.str());
        m_Orbiters[m_PlayerShip->GetOrbitSystemId()] = m_PlayerShip;

        m_CameraHostId = planet0->GetOrbitSystemId(); // set camera orbit space to Planet 0
        m_CameraTrackingId = m_PlayerShip->GetOrbitSystemId(); // Track Player Ship

        //id = orbs.CreateOrbiterCS(false, true, LV::BigFloat(1.f, 2.f), planet0Id, LV::Vector2(-0.7f, 0.f), false);
        //nameoss.str(""); nameoss << "Practice Target 0 (" << id << ")";
        //m_OrbiterRenderInfo[id] = { nameoss.str(), 0.00003f, {0.7f, 0.4f, 0.3f, 1.f} };
    }

    InflOrbRef planet1 = InfluencingOrbiter::Create("Planet 1", 0.001f, { 0.2f, 0.7f, 1.f, 1.f },
        LV::BigFloat(1.f, 6), 0, LV::Vector2(0.f, -.5f), false);
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

    // Player input data - relevant to both Update and Render scopes
    LV::Vector2 shipPos, shipToMouseLine;
    bool shipIsBeingControlled, shipIsThrusting;

    // Update
    {
        LV_PROFILE_SCOPE("Update - Dev2DLayer::OnUpdate");

        // Player Ship input
        static constexpr float shipAcceleration = 0.5f;

        shipIsBeingControlled = !m_CameraController->IsBeingControlled();//&& m_CameraTrackingId == m_PlayerShipId;
        if (shipIsBeingControlled)
        {
            // Get line from Player Ship to mouse position
            float mouseX, mouseY;
            std::tie(mouseX, mouseY) = LV::Input::GetMousePosition();
            auto mousePos = m_CameraController->GetWorldXY({ mouseX, mouseY });
            LV::Vector2 hostPos{ 0.f, 0.f };
            shipPos = orbs.GetParameters(m_PlayerShip->GetOrbitSystemId()).Position;
            if (m_CameraTrackingId == m_PlayerShip->GetOrbitSystemId())
            {
                hostPos = -shipPos;
                shipPos = { 0.f, 0.f };
            }
            else if (m_CameraTrackingId != m_CameraHostId)
            {
                hostPos = -orbs.GetParameters(m_CameraTrackingId).Position;
                shipPos += hostPos;
            }
            shipToMouseLine = mousePos - shipPos;

            // On left-click, apply acceleration along ship-mouse vector
            shipIsThrusting = LV::Input::IsMouseButtonPressed(LV_MOUSE_BUTTON_LEFT) && shipToMouseLine.SqrMagnitude() > 0;
            if (shipIsThrusting)
            {
                LV::BigVector2 acceleration{ shipAcceleration * shipToMouseLine.Normalized() };
                orbs.AccelerateOrbiter(m_PlayerShip->GetOrbitSystemId(), acceleration);
            }
        }
        else
        {
            shipToMouseLine = LV::Vector2::Zero();
        }

        // Orbit system
        orbs.Update(dT);

        // Camera
        if (m_CameraTrackingChanged)
        {
            m_CameraController->SetXY(0);
            m_CameraTrackingChanged = false;
        }
        m_CameraController->OnUpdate(dT);
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
        static constexpr float baseIntersectCircleRadius = 0.016f;
        static constexpr float baseOrbiterCircleRadius = 0.024f;
        static constexpr float trackedSubOrbiterRadius = 0.001f;
        static constexpr float baseShipThrustLineThickness = 0.008f;
        static constexpr float circleLargeFillTexSizeFactor = 1280.f / 1270.f; // Texture widths per unit circle-DIAMETERS

        float zoom = m_CameraController->GetZoom();
        float trajectoryLineThickness = zoom * baseTrajectoryLineThickness;
        float intersectCircleRadius = zoom * baseIntersectCircleRadius;
        float orbiterCircleRadius = zoom * baseOrbiterCircleRadius;
        float shipThrustLineThickness = zoom * baseShipThrustLineThickness;

        // Get IDs of scene host and camera-tracked orbiter
        uint32_t sceneHostId = m_CameraTrackingId, cameraTrackingId = m_CameraTrackingId;
        for (uint32_t l = 0; l < m_CameraRelativeLevel; l++)
        {
            cameraTrackingId = sceneHostId;
            sceneHostId = orbs.GetHostId(sceneHostId);
        }
        bool cameraIsTrackingHost = cameraTrackingId == sceneHostId;

        auto& hostRef = m_Orbiters[sceneHostId];
        float hostScaling = orbs.GetScaling(sceneHostId);
        LV::Vector2 hostPos = cameraIsTrackingHost ? 0.f : -1.f * orbs.GetParameters(cameraTrackingId).Position;

        float hostQuadWidth = circleLargeFillTexSizeFactor * 2.f * hostRef->GetRadius() * hostScaling;
        LV::Renderer2D::DrawQuad(hostPos, { hostQuadWidth, hostQuadWidth }, m_CircleLargeFillTexture, hostRef->GetColor());

        // Get the orbiters of the scene host - these are all the orbiters in the same orbit space as the camera
        std::vector<uint32_t> visibleOrbiters;
        orbs.GetOrbiters(sceneHostId, visibleOrbiters);
        size_t numHostOrbiters = visibleOrbiters.size();

        // Get additional information about the orbiter being tracked by the camera
        bool trackedIsInfluencing = orbs.IsInfluencing(cameraTrackingId);
        float troi = trackedIsInfluencing ? orbs.GetRadiusOfInfluence(cameraTrackingId) : 0.f;
        if (!cameraIsTrackingHost)
        {
            // If the tracked orbiter is not the scene host, get its orbiters:
            // this allows the player to 'peek' into the tracked orbiter's orbit space
            if (trackedIsInfluencing)
            {
                orbs.GetOrbiters(cameraTrackingId, visibleOrbiters);
            }

            // As the camera-tracked orbiter is not also the camera host, we can draw the points where its orbit intersects with other orbits in the scene
            auto intCol = m_Orbiters[cameraTrackingId]->GetColor();
            intCol.x = powf(intCol.x + 0.1f, 2);
            intCol.y = powf(intCol.y + 0.1f, 2);
            intCol.z = powf(intCol.z + 0.1f, 2);
            intCol.w = 0.5f;
            for (auto& intersectsEntry : orbs.GetParameters(cameraTrackingId).Intersects)
            {
                for (uint32_t i = 0; i < intersectsEntry.second.first; i++)
                {
                    LV::Renderer2D::DrawQuad(hostPos + intersectsEntry.second.second[i], {circleThickTexSizeFactor * intersectCircleRadius}, m_CircleThickTexture, intCol);
                }
            }
        }

        // Render all visible orbiters and their influences
        for (size_t idx = 0; idx < visibleOrbiters.size(); idx++)
        {
            size_t orbId = visibleOrbiters[idx];

            bool cameraIsTrackingOrbiter = orbId == cameraTrackingId;
            bool orbiterIsInSceneSpace = idx < numHostOrbiters;

            auto& orbRef = m_Orbiters[orbId];
            auto& op = orbRef->GetParameters();

            LV::Vector2 orbPos = orbiterIsInSceneSpace ? (cameraIsTrackingOrbiter ? 0.f : hostPos + op.Position) : troi * op.Position;

            float hostRelativeScaling = orbiterIsInSceneSpace ? 1.f : troi;
            if (orbs.IsInfluencing(orbId))
            {
                float quadWidth = hostRelativeScaling * circleLargeFillTexSizeFactor * 2.f * orbs.GetRadiusOfInfluence(orbId);
                LV::Renderer2D::DrawQuad(orbPos, { quadWidth, quadWidth }, m_CircleLargeFillTexture, m_InfluenceColor);
            }

            if (orbiterIsInSceneSpace)
            {
                LV::Renderer2D::DrawQuad(orbPos, { circleFillTexSizeFactor * orbRef->GetRadius() * hostScaling}, m_CircleFillTexture, orbRef->GetColor());
                auto iconCol = orbRef->GetColor();
                iconCol.x = powf(iconCol.x + 0.1f, 2);
                iconCol.y = powf(iconCol.y + 0.1f, 2);
                iconCol.z = powf(iconCol.z + 0.1f, 2);
                iconCol.w = orbId == m_CameraTrackingId ? 0.7f : 0.3f;
                LV::Renderer2D::DrawQuad(orbPos, { circleThickTexSizeFactor * orbiterCircleRadius }, m_CircleThickTexture, iconCol);
            }
            else
            {
                LV::Renderer2D::DrawQuad(orbPos, { circleFillTexSizeFactor * trackedSubOrbiterRadius * hostScaling }, m_CircleFillTexture, orbRef->GetColor());
            }
        }

        // Draw line from Player Ship to mouse position
        if (shipIsBeingControlled)
        {
            auto shipInputUICol = m_PlayerShip->GetColor();
            shipInputUICol.x = powf(shipInputUICol.x + 0.1f, 2);
            shipInputUICol.y = powf(shipInputUICol.y + 0.1f, 2);
            shipInputUICol.z = powf(shipInputUICol.z + 0.1f, 2);
            shipInputUICol.w = shipIsThrusting ? 0.7f : 0.3f;
            LV::Renderer2D::DrawLine(shipPos, shipPos + shipToMouseLine, shipThrustLineThickness, shipInputUICol);
        }

        // Render linear trajectories
        for (size_t idx = 0; idx < visibleOrbiters.size(); idx++)
        {
            size_t orbId = visibleOrbiters[idx];
            bool orbiterIsInSceneSpace = idx < numHostOrbiters;
            bool cameraIsTrackingOrbiter = orbId == cameraTrackingId;

            auto& orbRef = m_Orbiters[orbId];
            auto& op = orbRef->GetParameters();

            if (op.NewtonianMotion)
            {
                LV::Vector2 centrePos = orbiterIsInSceneSpace ? hostPos : 0.f;
                LV::Vector2 orbPos = orbiterIsInSceneSpace ? (cameraIsTrackingOrbiter ? 0.f : op.Position) : troi * op.Position;

                auto col = orbRef->GetColor();
                col.w = cameraIsTrackingOrbiter ? 0.7f : 0.3f;
                LV::Renderer2D::DrawLine(centrePos, orbPos, trajectoryLineThickness, col);
            }
        }

        // Render elliptical orbits/trajectories
        LV::Renderer2D::TEMP_BeginEllipses(); // TEMPORARY: separate draw calls for different shaders - TODO: use render queue
        for (size_t idx = 0; idx < visibleOrbiters.size(); idx++)
        {
            size_t orbId = visibleOrbiters[idx];
            bool orbiterIsInSceneSpace = idx < numHostOrbiters;
            bool cameraIsTrackingOrbiter = orbId == cameraTrackingId;

            float hostRelativeScaling = orbiterIsInSceneSpace ? 1.f : troi;

            auto& orbRef = m_Orbiters[orbId];
            auto& op = orbRef->GetParameters();

            if (op.Type == OrbitSystem2D::OrbitType::Circle || op.Type == OrbitSystem2D::OrbitType::Ellipse)
            {
                LV::Vector2 centrePos = orbiterIsInSceneSpace ? hostPos + op.Centre : hostRelativeScaling * op.Centre;
                LV::Vector2 escapePointFromCentre{ 0.f, 0.f };
                if (op.TrueAnomalyEscape < LV::PI2f)
                {
                    float distanceCentreFocus = op.Eccentricity * op.SemiMajorAxis;
                    escapePointFromCentre = { distanceCentreFocus + op.EscapePointPerifocal.x, op.EscapePointPerifocal.y };
                    escapePointFromCentre *= hostRelativeScaling;
                }
                auto col = orbRef->GetColor();
                col.w = cameraIsTrackingOrbiter ? 0.7f : 0.3f;
                LV::Renderer2D::DrawEllipse(centrePos, op.RightAscensionPeriapsis, hostRelativeScaling * op.SemiMajorAxis, hostRelativeScaling * op.SemiMinorAxis,
                    escapePointFromCentre, trajectoryLineThickness, col);
            }
        }

        // Render hyperbolic trajectories
        LV::Renderer2D::TEMP_BeginHyperbolae(); // TEMPORARY: separate draw calls for different shaders - TODO: use render queue
        for (size_t idx = 0; idx < visibleOrbiters.size(); idx++)
        {
            size_t orbId = visibleOrbiters[idx];
            bool orbiterIsInSceneSpace = idx < numHostOrbiters;
            bool cameraIsTrackingOrbiter = orbId == cameraTrackingId;

            float hostRelativeScaling = orbiterIsInSceneSpace ? 1.f : troi;

            auto& orbRef = m_Orbiters[orbId];
            auto& op = orbRef->GetParameters();

            if (op.Type == OrbitSystem2D::OrbitType::Hyperbola)
            {
                LV::Vector2 centrePos = idx < numHostOrbiters ? hostPos + op.Centre : hostRelativeScaling * op.Centre;

                float distanceCentreFocus = op.Eccentricity * op.SemiMajorAxis;
                LV::Vector2 escapePointFromCentre{ distanceCentreFocus - op.EscapePointPerifocal.x, op.EscapePointPerifocal.y };
                escapePointFromCentre *= hostRelativeScaling;

                auto col = orbRef->GetColor();
                col.w = orbId == m_CameraTrackingId ? 0.7f : 0.3f;
                LV::Renderer2D::DrawHyperbola(centrePos, op.RightAscensionPeriapsis, hostRelativeScaling * op.SemiMajorAxis, hostRelativeScaling * op.SemiMinorAxis,
                    escapePointFromCentre, trajectoryLineThickness, col);
            }
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
    uint32_t sceneHostId = m_CameraTrackingId, cameraTrackingId = m_CameraTrackingId;
    for (uint32_t l = 0; l < m_CameraRelativeLevel; l++)
    {
        cameraTrackingId = sceneHostId;
        sceneHostId = orbs.GetHostId(sceneHostId);
    }
    bool cameraIsTrackingHost = cameraTrackingId == sceneHostId;

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

    // Camera host selection
    std::vector<uint32_t> hostIds;
    orbs.GetAllHosts(hostIds);
    if (ImGui::BeginCombo("Scene Host", m_Orbiters[sceneHostId]->GetName().c_str()))
    {
        for (uint32_t id : hostIds)
        {
            const bool isSelected = (m_CameraHostId == id);
            if (ImGui::Selectable(m_Orbiters[id]->GetName().c_str(), isSelected))
            {
                m_CameraHostId = id;
                m_CameraTrackingId = id;
                m_CameraTrackingChanged = true;
                m_CameraController->SetZoom(1.f);
            }

            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if (isSelected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    auto& host = orbs.GetHost(m_CameraHostId);

    // Camera tracking orbiter selection
    if (ImGui::BeginCombo("Orbiter Tracking", m_Orbiters[cameraTrackingId]->GetName().c_str()))
    {
        for (uint32_t id : trackableOrbiterIds)
        {
            const bool isSelected = (m_CameraTrackingId == id);
            if (ImGui::Selectable(m_Orbiters[id]->GetName().c_str(), isSelected))
            {
                m_CameraTrackingId = id;
                m_CameraTrackingChanged = true;
            }

            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if (isSelected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

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
    float scaling = 0 == m_CameraHostId ? 1.f : host.GetHostScaling();
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
    m_CameraController->OnEvent(e);
}
