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

    // Update
    {
        LV_PROFILE_SCOPE("Update - Dev2DLayer::OnUpdate");

        OrbitSystem2D::Get().Update(dT);

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

        OrbitSystem2D& orbs = OrbitSystem2D::Get();
        constexpr float orbiterTxpr = 4.f; // Orbiter texture scaling factor: texture widths per unit orbiter-radius
        constexpr float orbitTxpr = 2.f * 1280.f / 1270.f; // Orbit texture scaling factor: texture widths per texture semi-major axis

        auto& starParams = orbs.GetHostRenderInfo();
        Limnova::Renderer2D::DrawQuad(starParams.Position, { orbiterTxpr * 0.1f }, m_CircleFillTexture, { 0.9f, 1.f, 1.f, 1.f });

        // Orb0
        auto& orb0Params = orbs.GetParameters(m_Orb0Id);
        Limnova::Renderer2D::DrawQuad(orb0Params.Position, { orbiterTxpr * 0.01f }, m_CircleFillTexture, m_Orb0Color);
        Limnova::Renderer2D::DrawRotatedQuad(starParams.Position + orb0Params.Centre,
            orbitTxpr * Limnova::Vector2(orb0Params.SemiMajorAxis, orb0Params.SemiMinorAxis),
            orb0Params.RightAscensionPeriapsis, m_CircleTexture, { m_Orb0Color.x, m_Orb0Color.y, m_Orb0Color.z, .5f }
        );

        // Orb1
        auto& orb1Params = orbs.GetParameters(m_Orb1Id);
        Limnova::Renderer2D::DrawQuad(orb1Params.Position, { orbiterTxpr * 0.01f }, m_CircleFillTexture, m_Orb1Color);
        Limnova::Renderer2D::DrawRotatedQuad(starParams.Position + orb1Params.Centre,
            orbitTxpr * Limnova::Vector2(orb1Params.SemiMajorAxis, orb1Params.SemiMinorAxis),
            orb1Params.RightAscensionPeriapsis, m_CircleTexture, { m_Orb1Color.x, m_Orb1Color.y, m_Orb1Color.z, .5f }
        );

        Limnova::Renderer2D::EndScene();
    }
}


void Orbiters2D::OnImGuiRender()
{
    ImGui::Begin("Orbiters2D");

    ImGui::BeginTable("Orbiters", 3);

    OrbitSystem2D& orbs = OrbitSystem2D::Get();
    auto& op0 = orbs.GetParameters(m_Orb0Id);
    auto& op1 = orbs.GetParameters(m_Orb1Id);

    // Row 0
    /*ImGui::TableNextRow();

    ImGui::TableSetColumnIndex(0);
    ImGui::Text("HUD Color");
    ImGui::TableSetColumnIndex(1);
    ImGui::ColorEdit4("Orb0", glm::value_ptr(*(glm::vec4*)&m_Orb0Color));
    ImGui::TableSetColumnIndex(2);
    ImGui::ColorEdit4("Orb1", glm::value_ptr(*(glm::vec4*)&m_Orb0Color));*/

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

    ImGui::EndTable();

    ImGui::End();
}


void Orbiters2D::OnEvent(Limnova::Event& e)
{
    m_CameraController->OnEvent(e);
}
