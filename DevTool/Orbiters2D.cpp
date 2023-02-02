#include "Orbiters2D.h"

#include "OrbitSystem2D.h"

#define ASSET_DIR "C:\\Programming\\source\\Limnova\\DevTool\\assets"


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
    Limnova::Application& app = Limnova::Application::Get();
    m_CameraController = std::make_shared<Limnova::OrthographicPlanarCameraController>(
        Limnova::Vector3(0.f, 0.f, 2.f), Limnova::Vector3(0.f, 0.f, -1.f),
        (float)app.GetWindow().GetWidth() / (float)app.GetWindow().GetHeight(), 0.1f, 100.f
    );
    m_CameraController->SetZoomLimits(kZoomMin, kZoomMax);

    // OrbitSystem
    OrbitSystem2D::Init();
    OrbitSystem2D& orbs = OrbitSystem2D::Get();
    m_Timescale = 0.01;
    orbs.SetTimeScale(m_Timescale);

    orbs.SetOrbiterEventCallback([&](const uint32_t id)
    {
        if (m_CameraTrackingId == id &&
            m_CameraHostId != m_CameraTrackingId)
        {
            m_CameraHostId = orbs.GetOrbiterHost(id);
            m_CameraTrackingChanged = true; // Re-centre camera on tracked orbiter
            m_CameraController->SetZoom(kZoomMin);
        }
    });

    orbs.LoadLevel({ 1.498284464f, 10 }, { 1.f, 0 }); // Host mass initialised to inverse of gravitational constant --> M_host * G = 1
    m_CameraHostId = 0;
    m_CameraTrackingId = 0;
    m_OrbiterRenderInfo[0] = { "Star (0)", 0.05f, {0.9f, 1.f, 1.f, 1.f}};
    uint32_t id;
    std::ostringstream nameoss;
    id = orbs.CreateOrbiterES(true, false, Limnova::BigFloat(2.f, 6), 0, Limnova::Vector2(1.f, 0.f), Limnova::BigVector2(-0.3f, 1.f));
    nameoss.str(""); nameoss << "Planet 0 (" << id << ")";
    m_OrbiterRenderInfo[id] = { nameoss.str(), 0.001f, {0.2f, 0.3f, 1.f, 1.f}};
    //m_CameraHostId = id; // set camera orbit space to Planet 0
    m_CameraTrackingId = id; // track Planet 0
    id = orbs.CreateOrbiterCS(true, false, Limnova::BigFloat(1.f, 2), id, Limnova::Vector2(0.f, 0.9f), false);
    nameoss.str(""); nameoss << "Moon 0.0 (" << id << ")";
    m_OrbiterRenderInfo[id] = { nameoss.str(), 0.00005f, {0.3f, 0.9f, 1.f, 1.f}};
    id = orbs.CreateOrbiterCU(true, false, Limnova::BigFloat(1.5f, 2), Limnova::BigVector2(1.02f, 0.f), true);
    nameoss.str(""); nameoss << "Moon 0.1 (" << id << ")";
    m_OrbiterRenderInfo[id] = { nameoss.str(), 0.00007f, {0.3f, 0.9f, 0.2f, 1.f}};

    id = orbs.CreateOrbiterCS(true, false, Limnova::BigFloat(1.f, 6), 0, Limnova::Vector2(0.f, -.5f), false);
    nameoss.str(""); nameoss << "Planet 1 (" << id << ")";
    m_OrbiterRenderInfo[id] = { nameoss.str(), 0.001f, {0.2f, 0.7f, 1.f, 1.f}};
    id = orbs.CreateOrbiterCS(true, false, Limnova::BigFloat(1.f, 2), id, Limnova::Vector2(0.f, -.7f), false);
    nameoss.str(""); nameoss << "Moon 1.0 (" << id << ")";
    m_OrbiterRenderInfo[id] = { nameoss.str(), 0.00003f, {0.5f, 0.2f, .3f, 1.f}};

    // Testing dynamic orbits - orbiters moving between hosts at runtime
    id = orbs.CreateOrbiterES(false, true, Limnova::BigFloat(1.f, 2), 0, Limnova::Vector2(1.01f, 0.f), Limnova::BigVector2(0.f - 0.3f, 0.15f + 1.f));
    nameoss.str(""); nameoss << "Comet 0 (" << id << ")";
    m_OrbiterRenderInfo[id] = { nameoss.str(), 0.00003f, {0.3f, 0.9f, 1.f, 1.f}};
    id = orbs.CreateOrbiterES(false, true, Limnova::BigFloat(1.f, 2), 0, Limnova::Vector2(0.96f, 0.f), Limnova::BigVector2(-0.13f, 0.95f));
    nameoss.str(""); nameoss << "Comet 1 (" << id << ")";
    m_OrbiterRenderInfo[id] = { nameoss.str(), 0.00003f, {1.f, 0.9f, 0.3f, 1.f}};
    id = orbs.CreateOrbiterES(false, true, Limnova::BigFloat(1.f, 2), 1, Limnova::Vector2(0.f, 0.3f), Limnova::BigVector2(-4.9f, 0.f));
    nameoss.str(""); nameoss << "Comet 2 (" << id << ")";
    m_OrbiterRenderInfo[id] = { nameoss.str(), 0.00003f, {1.f, 0.5f, 0.2f, 1.f}};

    // Testing hyperbolic trajectories
    id = orbs.CreateOrbiterES(false, true, Limnova::BigFloat(1.f, 2), 0, Limnova::Vector2(0.99f, 0.f), Limnova::BigVector2(-0.3f, 0.8));
    nameoss.str(""); nameoss << "Comet 3 (" << id << ")";
    m_OrbiterRenderInfo[id] = { nameoss.str(), 0.00003f, {0.3f, 0.9f, 1.f, 1.f}};

    // Textures
    m_CheckerboardTexture = Limnova::Texture2D::Create(ASSET_DIR"\\textures\\testtex.png", Limnova::Texture::WrapMode::MirroredTile);
    m_CircleFillTexture = Limnova::Texture2D::Create(ASSET_DIR"\\textures\\orbiter-0.png", Limnova::Texture::WrapMode::Clamp);
    m_CircleTexture = Limnova::Texture2D::Create(ASSET_DIR"\\textures\\orbit-a1270.png", Limnova::Texture::WrapMode::Clamp);
}


void Orbiters2D::OnDetach()
{
    OrbitSystem2D::Get().Shutdown();
}


void Orbiters2D::OnUpdate(Limnova::Timestep dT)
{
    LV_PROFILE_FUNCTION();

    OrbitSystem2D& orbs = OrbitSystem2D::Get();

    // Update
    {
        LV_PROFILE_SCOPE("Update - Dev2DLayer::OnUpdate");

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

        Limnova::RenderCommand::SetClearColor({ 0.f, 0.f, 0.f, 1.f });
        Limnova::RenderCommand::Clear();
    }

    {
        LV_PROFILE_SCOPE("Render Draw - Orbiters2D::OnUpdate");

        Limnova::Renderer2D::BeginScene(m_CameraController->GetCamera());

        static constexpr float circleFillTexSizefactor = 4.f; // Texture widths per unit circle-RADII
        static constexpr float circleTexSizefactor = 2.f * 1280.f / 1270.f; // Texture widths per unit circle-DIAMETERS
        static constexpr float baseTrajectoryLineThickness = 0.008f;
        static constexpr float baseIntersectCircleRadius = 0.016f;
        static constexpr float trackedSubOrbiterRadius = 0.001f;

        float zoom = m_CameraController->GetZoom();
        float trajectoryLineThickness = zoom * baseTrajectoryLineThickness;
        float intersectCircleRadius = zoom * baseIntersectCircleRadius;

        // Render camera's local host
        auto& host = orbs.GetHost(m_CameraHostId);
        auto& hp = host.GetParameters();
        auto& rih = m_OrbiterRenderInfo[m_CameraHostId];

        float drawScaling = host.GetScaling();

        Limnova::Vector2 hostPos = m_CameraTrackingId == m_CameraHostId ? 0.f : -1.f * orbs.GetParameters(m_CameraTrackingId).Position;
        Limnova::Renderer2D::DrawQuad(hostPos, { circleFillTexSizefactor * rih.Radius * drawScaling }, m_CircleFillTexture, rih.Color);

        // Render visible orbiters and their influences
        std::vector<uint32_t> visibleOrbiters;
        host.GetChildren(visibleOrbiters);

        bool trackedIsInfluencing = orbs.IsInfluencing(m_CameraTrackingId);
        float troi = trackedIsInfluencing ? orbs.GetRadiusOfInfluence(m_CameraTrackingId) : 0.f;

        size_t numCameraHostOrbiters = visibleOrbiters.size();
        if (m_CameraTrackingId != m_CameraHostId)
        {
            for (auto& intersect : orbs.GetParameters(m_CameraTrackingId).Intersects)
            {
                for (uint32_t i = 0; i < intersect.second.first; i++)
                {
                    auto intCol = m_OrbiterRenderInfo[intersect.first].Color;
                    intCol.w = 0.5f;
                    Limnova::Renderer2D::DrawQuad(hostPos + intersect.second.second[i], {circleFillTexSizefactor * intersectCircleRadius}, m_CircleFillTexture, intCol);
                }
            }

            if (trackedIsInfluencing)
            {
                orbs.GetOrbiters(m_CameraTrackingId, visibleOrbiters);
            }
        }

        for (size_t idx = 0; idx < visibleOrbiters.size(); idx++)
        {
            size_t orbId = visibleOrbiters[idx];

            auto& op = orbs.GetParameters(orbId);
            auto& ri = m_OrbiterRenderInfo[orbId];
            Limnova::Vector2 orbPos = idx < numCameraHostOrbiters ? (orbId == m_CameraTrackingId ? 0.f : hostPos + op.Position) : troi * op.Position;
            float orbRadius = idx < numCameraHostOrbiters ? ri.Radius : trackedSubOrbiterRadius;
            Limnova::Renderer2D::DrawQuad(orbPos, { circleFillTexSizefactor * orbRadius * drawScaling }, m_CircleFillTexture, ri.Color);
            float hostRelativeScaling = idx < numCameraHostOrbiters ? 1.f : troi;
            if (orbs.IsInfluencing(orbId))
            {
                Limnova::Renderer2D::DrawQuad(orbPos, { hostRelativeScaling * circleFillTexSizefactor * orbs.GetRadiusOfInfluence(orbId) }, m_CircleFillTexture, m_InfluenceColor);
            }
        }

        // Render elliptical orbits/trajectories
        Limnova::Renderer2D::TEMP_BeginEllipses(); // TEMPORARY: separate draw calls for different shaders - TODO: use render queue
        for (size_t idx = 0; idx < visibleOrbiters.size(); idx++)
        {
            size_t orbId = visibleOrbiters[idx];
            float hostRelativeScaling = idx < numCameraHostOrbiters ? 1.f : troi;

            auto& op = orbs.GetParameters(orbId);
            auto& ri = m_OrbiterRenderInfo[orbId];
            if (op.Type == OrbitSystem2D::OrbitType::Circle || op.Type == OrbitSystem2D::OrbitType::Ellipse)
            {
                Limnova::Vector2 centrePos = idx < numCameraHostOrbiters ? hostPos + op.Centre : hostRelativeScaling * op.Centre;
                Limnova::Vector2 escapePointFromCentre{ 0.f, 0.f };
                if (op.TrueAnomalyEscape < Limnova::PI2f)
                {
                    float distanceCentreFocus = op.Eccentricity * op.SemiMajorAxis;
                    escapePointFromCentre = { distanceCentreFocus + op.EscapePointPerifocal.x, op.EscapePointPerifocal.y };
                    escapePointFromCentre *= hostRelativeScaling;
                }
                Limnova::Renderer2D::DrawEllipse(centrePos, op.RightAscensionPeriapsis, hostRelativeScaling * op.SemiMajorAxis, hostRelativeScaling * op.SemiMinorAxis,
                    escapePointFromCentre, trajectoryLineThickness, { ri.Color.x, ri.Color.y, ri.Color.z, .5f });
            }
        }

        // Render hyperbolic trajectories
        Limnova::Renderer2D::TEMP_BeginHyperbolae(); // TEMPORARY: separate draw calls for different shaders - TODO: use render queue
        for (size_t idx = 0; idx < visibleOrbiters.size(); idx++)
        {
            size_t orbId = visibleOrbiters[idx];
            float hostRelativeScaling = idx < numCameraHostOrbiters ? 1.f : troi;

            auto& op = orbs.GetParameters(orbId);
            auto& ri = m_OrbiterRenderInfo[orbId];
            if (op.Type == OrbitSystem2D::OrbitType::Hyperbola)
            {
                Limnova::Vector2 centrePos = idx < numCameraHostOrbiters ? hostPos + op.Centre : hostRelativeScaling * op.Centre;

                float distanceCentreFocus = op.Eccentricity * op.SemiMajorAxis;
                Limnova::Vector2 escapePointFromCentre{ distanceCentreFocus - op.EscapePointPerifocal.x, op.EscapePointPerifocal.y };
                escapePointFromCentre *= hostRelativeScaling;

                Limnova::Renderer2D::DrawHyperbola(centrePos, op.RightAscensionPeriapsis, hostRelativeScaling * op.SemiMajorAxis, hostRelativeScaling * op.SemiMinorAxis,
                    escapePointFromCentre, trajectoryLineThickness, { ri.Color.x, ri.Color.y, ri.Color.z, .5f });
            }
        }

        Limnova::Renderer2D::EndScene();
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

    // Orbiter HUD colors
    std::vector<uint32_t> trackableOrbiterIds;
    trackableOrbiterIds.push_back(m_CameraHostId);
    orbs.GetOrbiters(m_CameraHostId, trackableOrbiterIds);
    for (uint32_t idx = 1; idx < trackableOrbiterIds.size(); idx++)
    {
        auto& ri = m_OrbiterRenderInfo[trackableOrbiterIds[idx]];
        ImGui::ColorEdit4(ri.Name.c_str(), glm::value_ptr(*(glm::vec4*)&ri.Color));
    }

    // Camera host selection
    std::vector<uint32_t> hostIds;
    orbs.GetAllHosts(hostIds);
    if (ImGui::BeginCombo("Scene Host", m_OrbiterRenderInfo[m_CameraHostId].Name.c_str()))
    {
        for (uint32_t n : hostIds)
        {
            const bool isSelected = (m_CameraHostId == n);
            if (ImGui::Selectable(m_OrbiterRenderInfo[n].Name.c_str(), isSelected))
            {
                m_CameraHostId = n;
                m_CameraTrackingId = n;
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
    if (ImGui::BeginCombo("Orbiter Tracking", m_OrbiterRenderInfo[m_CameraTrackingId].Name.c_str()))
    {
        for (uint32_t n : trackableOrbiterIds)
        {
            const bool isSelected = (m_CameraTrackingId == n);
            if (ImGui::Selectable(m_OrbiterRenderInfo[n].Name.c_str(), isSelected))
            {
                m_CameraTrackingId = n;
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
    ImGui::Text(m_OrbiterRenderInfo[m_CameraHostId].Name.c_str());
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
        ImGui::Text(m_OrbiterRenderInfo[id].Name.c_str());
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


void Orbiters2D::OnEvent(Limnova::Event& e)
{
    m_CameraController->OnEvent(e);
}
