#include "Play2DLayer.h"

#define ASSET_DIR "C:\\Programming\\source\\Limnova\\PlayApp\\Assets"


Play2DLayer::Play2DLayer()
    : Layer("Dev2D")
{
}


void Play2DLayer::OnAttach()
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
    m_SpriteSheet = Limnova::Texture2D::Create(ASSET_DIR"\\textures\\kenney-sheet\\Spritesheet\\RPGpack_sheet_2X.png", Limnova::Texture::WrapMode::Clamp);
    m_SpriteStairs = Limnova::SubTexture2D::CreateFromCoords(m_SpriteSheet, { 7, 6 }, { 128, 128 });
    m_SpriteTree = Limnova::SubTexture2D::CreateFromCoords(m_SpriteSheet, { 2, 1 }, { 128, 128 }, { 1, 2 });
}


void Play2DLayer::OnDetach()
{
    LV_PROFILE_FUNCTION();
}


void Play2DLayer::OnUpdate(Limnova::Timestep dT)
{
    LV_PROFILE_FUNCTION();

    static float s_AnimatedRotation;
    constexpr float rotationSpeed = 30.f;

    // Update
    {
        LV_PROFILE_SCOPE("m_CameraController->OnUpdate - Play2DLayer::OnUpdate");

        m_CameraController->OnUpdate(dT);

        s_AnimatedRotation = Limnova::Wrapf(s_AnimatedRotation + dT * rotationSpeed, 0.f, 360.f);
    }

    // Render
    Limnova::Renderer2D::ResetStatistics();
    {
        LV_PROFILE_SCOPE("Render Prep - Play2DLayer::OnUpdate");

        Limnova::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.f });
        Limnova::RenderCommand::Clear();
    }

    {
        LV_PROFILE_SCOPE("Render Draw - Play2DLayer::OnUpdate");

        // Scene 1 - test quads
        Limnova::Renderer2D::BeginScene(m_CameraController->GetCamera());
        Limnova::Renderer2D::DrawRotatedQuad({ 0.f, 0.f }, { 3.f, 3.f }, glm::radians(m_BackgroundRotation), m_CheckerboardTexture, m_TextureTint, m_TextureScale);
        Limnova::Renderer2D::DrawRotatedQuad({ 0.f, 0.5f, 1.f }, { 0.5f, 0.5f }, glm::radians(s_AnimatedRotation), m_TextureTint);
        Limnova::Renderer2D::DrawQuad({ 0.75f, 0.f, 0.5f }, { 1.5f, 1.f }, m_SquareColor);
        Limnova::Renderer2D::EndScene();

        // Scene 2 - procedural grid
        Limnova::Renderer2D::BeginScene(m_CameraController->GetCamera());
        for (float y = -1.5f + 0.125f; y < 1.5f; y += 0.125f)
        {
            for (float x = -1.5f + 0.125f; x < 1.5f; x += 0.125f)
            {
                Limnova::Vector4 color{ (x + 1.5f) / 3.f, 0.5f, (y + 1.5f) / 3.f, 0.7f };
                Limnova::Renderer2D::DrawQuad({ x, y, 0.f }, { 0.11f, 0.11f }, color);
            }
        }
        Limnova::Renderer2D::EndScene();

        // Scene 3 - sprites
        Limnova::Renderer2D::BeginScene(m_CameraController->GetCamera());
        Limnova::Renderer2D::DrawQuad({ 0.f, -1.f, 0.5f }, { 0.25f, 0.25f }, m_SpriteStairs);
        Limnova::Renderer2D::DrawQuad({ -0.25f, -1.f, 0.5f }, { 0.25f, 0.5f }, m_SpriteTree);
        Limnova::Renderer2D::EndScene();
    }
}


void Play2DLayer::OnImGuiRender()
{
    ImGui::Begin("Scene Properties");
    ImGui::ColorEdit4("Square Color", glm::value_ptr(*(glm::vec4*)&m_SquareColor));
    ImGui::ColorEdit4("Texture Tint", glm::value_ptr(*(glm::vec4*)&m_TextureTint));
    ImGui::SliderFloat2("Texture Scale", glm::value_ptr(*(glm::vec2*)&m_TextureScale), 0.1f, 10.f);
    ImGui::SliderFloat("BackgroundRotation", &m_BackgroundRotation, 0.f, 360.f);
    ImGui::End(); // Scene Properties

    ImGui::Begin("Renderer2D Statistics");
    auto& stats = Limnova::Renderer2D::GetStatistics();
    ImGui::Text("Draw Calls:    %d", stats.DrawCalls);
    ImGui::Text("Quads:         %d", stats.QuadCount);
    ImGui::Text("Vertices:      %d", stats.GetNumVertices());
    ImGui::Text("Indices:       %d", stats.GetNumIndices());
    ImGui::End(); // Renderer2D Statistics
}


void Play2DLayer::OnEvent(Limnova::Event& e)
{
    m_CameraController->OnEvent(e);
}

