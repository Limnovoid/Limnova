#include "Dev2D.h"

#define ASSET_DIR "C:\\Programming\\source\\Limnova\\DevTool\\assets"


Dev2DLayer::Dev2DLayer()
    : Layer("Dev2D")
{
}


void Dev2DLayer::OnAttach()
{
    LV_PROFILE_FUNCTION();

    // Camera
    Limnova::Application& app = Limnova::Application::Get();
    m_CameraController = std::make_shared<Limnova::PerspectivePlanarCameraController>(
        Limnova::Vector3(0.f, 0.f, 2.f), Limnova::Vector3(0.f, 0.f, -1.f),
        (float)app.GetWindow().GetWidth() / (float)app.GetWindow().GetHeight(),
        0.1f, 100.f, glm::radians(60.f)
    );
    m_CameraController->SetControlled(true);

    /*m_CameraController = std::make_shared<Limnova::OrthographicPlanarCameraController>(
        Limnova::Vector3(0.f, 0.f, 1.f), Limnova::Vector3(0.f, 0.f, -1.f),
        (float)app.GetWindow().GetWidth() / (float)app.GetWindow().GetHeight(),
        0.1f, 100.f
    );*/

    // Textures
    m_TurretTexture = Limnova::Texture2D::Create(ASSET_DIR"\\textures\\turret.png", Limnova::Texture::WrapMode::Clamp);
    m_CheckerboardTexture = Limnova::Texture2D::Create(ASSET_DIR"\\textures\\testtex.png", Limnova::Texture::WrapMode::MirroredTile);
}


void Dev2DLayer::OnDetach()
{
    LV_PROFILE_FUNCTION();
}


void Dev2DLayer::OnUpdate(Limnova::Timestep dT)
{
    LV_PROFILE_FUNCTION();

    static float s_AnimatedRotation;
    constexpr float rotationSpeed = 30.f;

    // Update
    {
        LV_PROFILE_SCOPE("m_CameraController->OnUpdate - Dev2DLayer::OnUpdate");

        m_CameraController->OnUpdate(dT);

        s_AnimatedRotation = Limnova::Wrap(s_AnimatedRotation + dT * rotationSpeed, 0.f, 360.f);
    }

    // Render
    {
        LV_PROFILE_SCOPE("Render Prep - Dev2DLayer::OnUpdate");

        Limnova::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.f });
        Limnova::RenderCommand::Clear();
    }

    {
        LV_PROFILE_SCOPE("Render Draw - Dev2DLayer::OnUpdate");

        Limnova::Renderer2D::BeginScene(m_CameraController->GetCamera());
        Limnova::Renderer2D::DrawRotatedQuad({ 0.f, 0.f }, { 3.f, 3.f }, glm::radians(m_BackgroundRotation), m_CheckerboardTexture, m_TextureTint, m_TextureScale);
        //Limnova::Renderer2D::DrawQuad({ 0.f, 0.f }, { 3.f, 3.f }, m_CheckerboardTexture, m_TextureTint, m_TextureScale);
        Limnova::Renderer2D::DrawRotatedQuad({ 0.f, 0.5f, 1.f }, { 0.5f, 0.5f }, glm::radians(s_AnimatedRotation), m_TextureTint);
        Limnova::Renderer2D::DrawQuad({ 0.75f, 0.f, 0.5f }, { 1.5f, 1.f }, m_SquareColor);

        Limnova::Renderer2D::EndScene();
    }
}


void Dev2DLayer::OnImGuiRender()
{
    ImGui::Begin("Scene");
    ImGui::ColorEdit4("Square Color", glm::value_ptr(*(glm::vec4*)&m_SquareColor));
    ImGui::ColorEdit4("Texture Tint", glm::value_ptr(*(glm::vec4*)&m_TextureTint));
    ImGui::SliderFloat2("Texture Scale", glm::value_ptr(*(glm::vec2*)&m_TextureScale), 0.1f, 10.f);
    ImGui::SliderFloat("BackgroundRotation", &m_BackgroundRotation, 0.f, 360.f);
    ImGui::End();
}


void Dev2DLayer::OnEvent(Limnova::Event& e)
{
    m_CameraController->OnEvent(e);
}

