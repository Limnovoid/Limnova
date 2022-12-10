#include "Orbiters2D.h"

#include "OrbitSystem2D.h"

#define ASSET_DIR "C:\\Programming\\source\\Limnova\\DevTool\\assets"


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

    // OrbitSystem
    OrbitSystem2D::Init();
    OrbitSystem2D& orbs = OrbitSystem2D::Get();
    orbs.SetTimeScale(m_Timescale);

    orbs.LoadLevel({ 1.498284464f, 10 }, { 1.f, 0 }); // Host mass initialised to inverse of gravitational constant - GM = 1
    m_CameraHostId = 0;
    m_OrbiterRenderInfo[m_CameraHostId] = { "Star", 0.1f, {0.9f, 1.f, 1.f, 1.f}};
    uint32_t id;
    id = orbs.CreateOrbiter({ 1.f, 6 }, { 1.f, 0.f }, { -0.3f, 1.f });
    m_OrbiterRenderInfo[id] = { "Planet 0", 0.01f, {0.2f, 0.3f, 1.f, 1.f}, true, true};
    id = orbs.CreateOrbiter({ 1.f, 2 }, { 1.f, 0.02f }, false);
    m_OrbiterRenderInfo[id] = { "Moon 0", 0.0003f, {0.3f, 0.9f, 1.f, 1.f}, true, true };
    id = orbs.CreateOrbiter({ 1.f, 6 }, { 0.f, -.5f }, false);
    m_OrbiterRenderInfo[id] = { "Planet 1", 0.01f, {0.2f, 0.7f, 1.f, 1.f}, true, true};

    // Textures
    m_CheckerboardTexture = Limnova::Texture2D::Create(ASSET_DIR"\\textures\\testtex.png", Limnova::Texture::WrapMode::MirroredTile);
    m_CircleFillTexture = Limnova::Texture2D::Create(ASSET_DIR"\\textures\\orbiter-0.png", Limnova::Texture::WrapMode::Clamp);
    m_CircleTexture = Limnova::Texture2D::Create(ASSET_DIR"\\textures\\orbit-a1270.png", Limnova::Texture::WrapMode::Clamp);
}


void Orbiters2D::OnDetach()
{

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

        constexpr float circleFillTexSizefactor = 4.f; // Texture widths per unit circle-RADII
        constexpr float circleTexSizefactor = 2.f * 1280.f / 1270.f; // Texture widths per unit circle-DIAMETERS

        // Render camera's local host
        auto& hp = orbs.GetParameters(m_CameraHostId);
        auto& rih = m_OrbiterRenderInfo[m_CameraHostId];
        float hostScaling = orbs.GetScaling(m_CameraHostId);

        // Keep tracked orbiter centred in scene - use its position to offset its host
        Limnova::Vector2 hostPos = m_CameraTrackingId == m_CameraHostId ? 0.f : -1.f * orbs.GetParameters(m_CameraTrackingId).Position;
        Limnova::Renderer2D::DrawQuad(hostPos, { circleFillTexSizefactor * rih.Radius * hostScaling }, m_CircleFillTexture, rih.Color);

        // Render host's orbiters
        std::vector<uint32_t> visibleOrbiters;
        orbs.GetChildren(m_CameraHostId, visibleOrbiters);
        for (auto orbId : visibleOrbiters)
        {
            auto& op = orbs.GetParameters(orbId);
            Limnova::Vector2 orbPos = orbId == m_CameraTrackingId ? 0.f : hostPos + op.Position;

            auto& ri = m_OrbiterRenderInfo[orbId];
            Limnova::Renderer2D::DrawQuad(orbPos, { circleFillTexSizefactor * ri.Radius * hostScaling }, m_CircleFillTexture, ri.Color);
            if (ri.DrawOrbit)
            {
                Limnova::Renderer2D::DrawRotatedQuad(hostPos + op.Centre,
                    circleTexSizefactor * Limnova::Vector2(op.SemiMajorAxis, op.SemiMinorAxis),
                    op.RightAscensionPeriapsis, m_CircleTexture, { ri.Color.x, ri.Color.y, ri.Color.z, .5f }
                );
            }
            if (ri.DrawInfluence)
            {
                Limnova::Renderer2D::DrawQuad(orbPos, { circleFillTexSizefactor * orbs.GetRadiusOfInfluence(orbId) }, m_CircleFillTexture, m_InfluenceColor);
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
    orbs.GetChildren(m_CameraHostId, trackableOrbiterIds);
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
    // Information
    float scaling = orbs.GetScaling(m_CameraHostId);
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
        ImGui::Text("%.2f (%.4f)", sqrtf(op.Velocity.SqrMagnitude()), sqrtf(op.Velocity.SqrMagnitude()) / scaling);
        ImGui::TableSetColumnIndex(3);
        ImGui::Text("%.4f (%.6f)", orbs.GetRadiusOfInfluence(id), orbs.GetRadiusOfInfluence(id) / scaling);
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
