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

    orbs.LoadLevel({ 1.498284464f, 10 }); // Inverse of gravitational constant
    m_Orb0Id = orbs.CreateOrbiter({ 1.f, 6 }, { 1.f, 0.f }, { -0.3f, 1.f });
    m_Orb1Id = orbs.CreateOrbiter({ 1.f, 6 }, { 0.f, -.5f }, false);
    
    orbs.SetOrbiterRightAscension(m_Orb0Id, 3.f * Limnova::PIover2f);

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
    auto& sp = orbs.GetLevelHostParams();
    auto& op0 = orbs.GetParameters(m_Orb0Id);
    auto& op1 = orbs.GetParameters(m_Orb1Id);

    // Update
    {
        LV_PROFILE_SCOPE("Update - Dev2DLayer::OnUpdate");

        orbs.Update(dT);

        // Camera
        Limnova::Vector2 newTrackingPosition;
        switch (m_CameraTrackingId)
        {
            case std::numeric_limits<uint32_t>::max():
                newTrackingPosition = sp.Position;      break;
            case 0: newTrackingPosition = op0.Position; break;
            case 1: newTrackingPosition = op1.Position; break;
        }
        if (m_CameraTrackingChanged)
        {
            m_CameraController->SetXY(newTrackingPosition);
            m_CameraTrackingChanged = false;
        }
        else
        {
            m_CameraController->TranslateXY(newTrackingPosition - m_CameraTrackingPosition);
        }
        m_CameraTrackingPosition = newTrackingPosition;
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

        Limnova::Renderer2D::DrawQuad(sp.Position, { circleFillTexSizefactor * 0.1f }, m_CircleFillTexture, { 0.9f, 1.f, 1.f, 1.f });

        // Orb0
        Limnova::Renderer2D::DrawQuad(op0.Position, { circleFillTexSizefactor * 0.01f }, m_CircleFillTexture, m_Orb0Color);
        Limnova::Renderer2D::DrawRotatedQuad(sp.Position + op0.Centre,
            circleTexSizefactor * Limnova::Vector2(op0.SemiMajorAxis, op0.SemiMinorAxis),
            op0.RightAscensionPeriapsis, m_CircleTexture, { m_Orb0Color.x, m_Orb0Color.y, m_Orb0Color.z, .5f }
        );
        Limnova::Renderer2D::DrawQuad(op0.Position, { circleFillTexSizefactor * op0.RadiusOfInfluence }, m_CircleFillTexture, m_InfluenceColor);

        // Orb1
        Limnova::Renderer2D::DrawQuad(op1.Position, { circleFillTexSizefactor * 0.01f }, m_CircleFillTexture, m_Orb1Color);
        Limnova::Renderer2D::DrawRotatedQuad(sp.Position + op1.Centre,
            circleTexSizefactor * Limnova::Vector2(op1.SemiMajorAxis, op1.SemiMinorAxis),
            op1.RightAscensionPeriapsis, m_CircleTexture, { m_Orb1Color.x, m_Orb1Color.y, m_Orb1Color.z, .5f }
        );
        Limnova::Renderer2D::DrawQuad(op1.Position, { circleFillTexSizefactor * op1.RadiusOfInfluence }, m_CircleFillTexture, m_InfluenceColor);

        Limnova::Renderer2D::EndScene();
    }
}


void Orbiters2D::OnImGuiRender()
{
    ImGui::Begin("Orbiters2D");

    OrbitSystem2D& orbs = OrbitSystem2D::Get();
    if (ImGui::SliderFloat("Timescale", &m_Timescale, 0.f, 10.f))
    {
        orbs.SetTimeScale(m_Timescale);
    }

    ImGui::ColorEdit4("Orb0", glm::value_ptr(*(glm::vec4*)&m_Orb0Color));
    ImGui::ColorEdit4("Orb1", glm::value_ptr(*(glm::vec4*)&m_Orb1Color));

    const char* items[] = { "Level Host", "Orbiter 0", "Orbiter 1" };
    static int selectedIdx = 0;
    if (ImGui::BeginCombo("Camera Tracking", items[selectedIdx]))
    {
        for (int n = 0; n < IM_ARRAYSIZE(items); n++)
        {
            const bool isSelected = (selectedIdx == n);
            if (ImGui::Selectable(items[n], isSelected))
            {
                selectedIdx = n;
                m_CameraTrackingId = n == 0 ? std::numeric_limits<uint32_t>::max() : n - 1;
                m_CameraTrackingChanged = true;
            }

            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if (isSelected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::BeginTable("Orbiters", 3);

    auto& op0 = orbs.GetParameters(m_Orb0Id);
    auto& op1 = orbs.GetParameters(m_Orb1Id);

    // Row 0
    ImGui::TableNextRow();
    ImGui::Text("Orbiter");
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("0");
    ImGui::TableSetColumnIndex(2);
    ImGui::Text("1");

    // Row 1
    ImGui::TableNextRow();

    ImGui::TableSetColumnIndex(0);
    ImGui::Text("True Anomaly");
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%.2f (%.2f)", op0.TrueAnomaly, glm::degrees(op0.TrueAnomaly));
    ImGui::TableSetColumnIndex(2);
    ImGui::Text("%.2f (%.2f)", op1.TrueAnomaly, glm::degrees(op1.TrueAnomaly));

    // Row 2
    ImGui::TableNextRow();

    ImGui::TableSetColumnIndex(0);
    ImGui::Text("Speed");
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%.2f", sqrtf(op0.Velocity.SqrMagnitude()));
    ImGui::TableSetColumnIndex(2);
    ImGui::Text("%.2f", sqrtf(op1.Velocity.SqrMagnitude()));

    // Row 2
    ImGui::TableNextRow();

    ImGui::TableSetColumnIndex(0);
    ImGui::Text("ROI");
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%.4f", op0.RadiusOfInfluence);
    ImGui::TableSetColumnIndex(2);
    ImGui::Text("%.4f", op1.RadiusOfInfluence);

    // Row 3
    ImGui::TableNextRow();

    ImGui::TableSetColumnIndex(0);
    ImGui::Text("Semi-major");
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%.4f", op0.SemiMajorAxis);
    ImGui::TableSetColumnIndex(2);
    ImGui::Text("%.4f", op1.SemiMajorAxis);

    ImGui::EndTable();

    ImGui::End();
}


void Orbiters2D::OnEvent(Limnova::Event& e)
{
    m_CameraController->OnEvent(e);
}
