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
    m_SpriteSheet = Limnova::Texture2D::Create(ASSET_DIR"\\textures\\kenney-sheet\\Spritesheet\\RPGpack_sheet_2X.png", Limnova::Texture::WrapMode::Clamp);
    m_SpriteStairs = Limnova::SubTexture2D::CreateFromCoords(m_SpriteSheet, { 7, 6 }, { 128, 128 });
    m_SpriteTree = Limnova::SubTexture2D::CreateFromCoords(m_SpriteSheet, { 2, 1 }, { 128, 128 }, { 1, 2 });
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
    Limnova::Renderer2D::ResetStatistics();
    {
        LV_PROFILE_SCOPE("Render Prep - Dev2DLayer::OnUpdate");

        Limnova::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.f });
        Limnova::RenderCommand::Clear();
    }

    {
        LV_PROFILE_SCOPE("Render Draw - Dev2DLayer::OnUpdate");

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


void Dev2DLayer::OnImGuiRender()
{
    // From imgui_demo.cpp /////////////////

    static bool dockspaceOpen = true;
    static bool opt_fullscreen = true;
    static bool opt_padding = false;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
    // because it would be confusing to have two docking targets within each others.
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen)
    {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }
    else
    {
        dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
    }

    // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
    // and handle the pass-thru hole, so we ask Begin() to not render a background.
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
    // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
    // all active windows docked into it will lose their parent and become undocked.
    // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
    // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
    if (!opt_padding)
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
    if (!opt_padding)
        ImGui::PopStyleVar();

    if (opt_fullscreen)
        ImGui::PopStyleVar(2);

    // Submit the DockSpace
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Exit", NULL, false))
            {
                Limnova::Application::Get().Close();
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }


    ImGui::Begin("Scene Properties");
    ImGui::ColorEdit4("Square Color", glm::value_ptr(*(glm::vec4*)&m_SquareColor));
    ImGui::ColorEdit4("Texture Tint", glm::value_ptr(*(glm::vec4*)&m_TextureTint));
    ImGui::SliderFloat2("Texture Scale", glm::value_ptr(*(glm::vec2*)&m_TextureScale), 0.1f, 10.f);
    ImGui::SliderFloat("BackgroundRotation", &m_BackgroundRotation, 0.f, 360.f);
    ImGui::End();

    ImGui::Begin("Renderer2D Statistics");
    auto& stats = Limnova::Renderer2D::GetStatistics();
    ImGui::Text("Draw Calls:    %d", stats.DrawCalls);
    ImGui::Text("Quads:         %d", stats.QuadCount);
    ImGui::Text("Vertices:      %d", stats.GetNumVertices());
    ImGui::Text("Indices:       %d", stats.GetNumIndices());
    ImGui::End();

    ImGui::Begin("Viewport");
    uint32_t viewportRendererId = m_CheckerboardTexture->GetRendererId();
    ImGui::Image((void*)viewportRendererId, ImVec2{128.f, 128.f});
    ImGui::End();

    ImGui::End();
}


void Dev2DLayer::OnEvent(Limnova::Event& e)
{
    m_CameraController->OnEvent(e);
}

