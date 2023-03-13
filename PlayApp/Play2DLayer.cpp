#include "Play2DLayer.h"

#define ASSET_DIR "C:\\Programming\\source\\Limnova\\PlayApp\\Assets"


namespace Limnova
{

    Play2DLayer::Play2DLayer()
        : Layer("Dev2D")
    {
    }


    void Play2DLayer::OnAttach()
    {
        LV_PROFILE_FUNCTION();

        m_Scene = CreateRef<Scene>();

        // Framebuffer
        FramebufferSpecification fbspec;
        fbspec.Width = 1280;
        fbspec.Height = 720;
        m_Framebuffer = Framebuffer::Create(fbspec);

        // Camera
        Application& app = Application::Get();
        m_CameraController = CreateRef<PerspectivePlanarCameraController>(
            Vector3(0.f, 0.f, 2.f), Vector3(0.f, 0.f, -1.f),
            (float)app.GetWindow().GetWidth() / (float)app.GetWindow().GetHeight(),
            0.1f, 100.f, glm::radians(60.f)
            );
        m_CameraController->SetControlled(true);

        /*m_CameraController = std::make_shared<OrthographicPlanarCameraController>(
            Vector3(0.f, 0.f, 1.f), Vector3(0.f, 0.f, -1.f),
            (float)app.GetWindow().GetWidth() / (float)app.GetWindow().GetHeight(),
            0.1f, 100.f
        );*/

        // Textures
        m_TurretTexture = Texture2D::Create(ASSET_DIR"\\textures\\turret.png", Texture::WrapMode::Clamp);
        m_CheckerboardTexture = Texture2D::Create(ASSET_DIR"\\textures\\testtex.png", Texture::WrapMode::MirroredTile);
        m_SpriteSheet = Texture2D::Create(ASSET_DIR"\\textures\\kenney-sheet\\Spritesheet\\RPGpack_sheet_2X.png", Texture::WrapMode::Clamp);
        m_SpriteStairs = SubTexture2D::CreateFromCoords(m_SpriteSheet, { 7, 6 }, { 128, 128 });
        m_SpriteTree = SubTexture2D::CreateFromCoords(m_SpriteSheet, { 2, 1 }, { 128, 128 }, { 1, 2 });

        Entity square = m_Scene->CreateEntity("Default Square");
        square.AddComponent<SpriteRendererComponent>(Vector4{ 0.2f, 1.f, 0.3f, 1.f });
        m_SquareEntity = square;

        m_Camera0 = m_Scene->CreateEntity("Camera 0");
        m_Camera0.AddComponent<PerspectiveCameraComponent>();
        {
            auto& transform = m_Camera0.GetComponent<TransformComponent>();
            transform.Set({ 1.f }, { 0.f, 0.f, -2.f });
        }

        m_Camera1 = m_Scene->CreateEntity("Camera 1");
        m_Camera1.AddComponent<PerspectiveCameraComponent>();
        {
            auto& transform = m_Camera1.GetComponent<TransformComponent>();
            transform.Set({ 1.f }, { 0.f, 0.f, -3.f });
        }

        m_Scene->SetActiveCamera(m_Camera0);
    }


    void Play2DLayer::OnDetach()
    {
        LV_PROFILE_FUNCTION();
    }


    void Play2DLayer::OnUpdate(Timestep dT)
    {
        LV_PROFILE_FUNCTION();

        static float s_AnimatedRotation;
        constexpr float rotationSpeed = 30.f;
        s_AnimatedRotation = Wrapf(s_AnimatedRotation + dT * rotationSpeed, 0.f, 360.f);

        m_CameraController->OnUpdate(dT);

        // Render
        Renderer2D::ResetStatistics();

        //m_Framebuffer->Bind();
        RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.f });
        RenderCommand::Clear();

        // Scene 1 - test quads
        /*Renderer2D::BeginScene(m_CameraController->GetCamera());
        Renderer2D::DrawRotatedQuad({ 0.f, 0.f }, { 3.f, 3.f }, glm::radians(m_BackgroundRotation), m_CheckerboardTexture, m_TextureTint, m_TextureScale);
        Renderer2D::DrawRotatedQuad({ 0.f, 0.5f, 1.f }, { 0.5f, 0.5f }, glm::radians(s_AnimatedRotation), m_TextureTint);
        Renderer2D::DrawQuad({ 0.75f, 0.f, 0.5f }, { 1.5f, 1.f }, m_SquareColor);
        Renderer2D::EndScene();*/

        // Scene 2 - procedural grid
        /*Renderer2D::BeginScene(m_CameraController->GetCamera());
        for (float y = -1.5f + 0.125f; y < 1.5f; y += 0.125f)
        {
            for (float x = -1.5f + 0.125f; x < 1.5f; x += 0.125f)
            {
                Vector4 color{ (x + 1.5f) / 3.f, 0.5f, (y + 1.5f) / 3.f, 0.7f };
                Renderer2D::DrawQuad({ x, y, 0.f }, { 0.11f, 0.11f }, color);
            }
        }
        Renderer2D::EndScene();*/

        // Scene 3 - sprites
        /*Renderer2D::BeginScene(m_CameraController->GetCamera());
        Renderer2D::DrawQuad({ 0.f, -1.f, 0.5f }, { 0.25f, 0.25f }, m_SpriteStairs);
        Renderer2D::DrawQuad({ -0.25f, -1.f, 0.5f }, { 0.25f, 0.5f }, m_SpriteTree);
        Renderer2D::EndScene();*/

        // Scene 4 - entities
        m_Scene->OnUpdate(dT);

        //m_Framebuffer->Unbind();
    }


    void Play2DLayer::OnImGuiRender()
    {
        ImGui::Begin("Scene Properties");

        if (m_SquareEntity)
        {
            ImGui::Separator();
            auto& tag = m_SquareEntity.GetComponent<TagComponent>();
            ImGui::Text("%s", tag.Tag.c_str());
            auto& color = m_SquareEntity.GetComponent<SpriteRendererComponent>().Color;
            ImGui::ColorEdit4("Square Color", color.Ptr());
            ImGui::Separator();
        }

        Entity activeCamera = m_Scene->GetActiveCamera();
        if (ImGui::BeginCombo("Camera", activeCamera.GetComponent<TagComponent>().Tag.c_str()))
        {
            std::vector<Entity> cameraEntities;
            m_Scene->GetEntitiesByComponents<PerspectiveCameraComponent>(cameraEntities);
            for (auto& entity : cameraEntities)
            {
                if (ImGui::Selectable(entity.GetComponent<TagComponent>().Tag.c_str(), activeCamera == entity))
                {
                    activeCamera = entity;
                    m_Scene->SetActiveCamera(entity);
                }
            }
            ImGui::EndCombo();
        }


        ImGui::ColorEdit4("Texture Tint", glm::value_ptr(*(glm::vec4*)&m_TextureTint));
        ImGui::SliderFloat2("Texture Scale", glm::value_ptr(*(glm::vec2*)&m_TextureScale), 0.1f, 10.f);
        ImGui::SliderFloat("BackgroundRotation", &m_BackgroundRotation, 0.f, 360.f);
        ImGui::End(); // Scene Properties

        ImGui::Begin("Renderer2D Statistics");
        auto& stats = Renderer2D::GetStatistics();
        ImGui::Text("Draw Calls:    %d", stats.DrawCalls);
        ImGui::Text("Quads:         %d", stats.QuadCount);
        ImGui::Text("Vertices:      %d", stats.GetNumVertices());
        ImGui::Text("Indices:       %d", stats.GetNumIndices());
        ImGui::End(); // Renderer2D Statistics
    }


    void Play2DLayer::OnEvent(Event& e)
    {
        EventDispatcher dispatcher{ e };
        dispatcher.Dispatch<WindowResizeEvent>(LV_BIND_EVENT_FN(Play2DLayer::OnWindowResize));
    }


    bool Play2DLayer::OnWindowResize(WindowResizeEvent& e)
    {
        m_Scene->OnEvent(e);
        return false;
    }

}
