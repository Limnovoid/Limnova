#include <Limnova.h>

#include "Platform/OpenGL/OpenGLShader.h" // TEMPORARY shader casting

#include <chrono> // TEMPORARY delta time

#define ASSET_DIR "C:\\Programming\\source\\Limnova\\DevTool\\assets"


class LIMNOVA_API DevLayer : public Limnova::Layer
{
public:
    DevLayer()
        : Layer("DevLayer")
    {
        // Camera
        Limnova::Application& app = Limnova::Application::Get();

        glm::vec3 pos = glm::vec3(0.f, 0.f, 1.f);
        glm::vec3 aim = glm::normalize(glm::vec3(0.f, 0.f, -1.f));
        glm::vec3 up = glm::vec3(0.f, 1.f, 0.f);

        float fov = glm::radians(60.f);
        float aspect = (float)app.GetWindow().GetWidth() / (float)app.GetWindow().GetHeight();
        float nearPlane = 0.1f;
        float farPlane = 100.f;

        m_TestCamera.reset(new Limnova::PointCamera(fov, aspect, nearPlane, farPlane));
        m_TestCamera->SetPosition(pos);
        m_TestCamera->SetAimDirection(aim);
        m_TestCamera->SetUpDirection(up);

        Limnova::Renderer::InitCameraBuffer(m_TestCamera);

        m_CameraPos = pos;
        m_CameraUp = { 0.f, 1.f, 0.f };
        m_CameraMoveSpeed = 1.f;
        m_MouseAimEnabled = false;
        app.GetWindow().SetRawMouseInput(true);
        m_CameraElevation = 0.f;
        m_CameraAzimuth = 0.f;
        std::tie(m_MouseX, m_MouseY) = Limnova::Input::GetMousePosition();
        m_MouseSensitivity = 0.1f;
        m_MaxElevation = 85.f;
        m_MinElevation = -85.f;

        // Vertex arrays
        /// Triangle
        m_VertexArray.reset(Limnova::VertexArray::Create());

        float vertices[3 * (3 + 4)] = {
            -0.5f, -0.5f, -1.f,     0.2f, 0.9f, 0.3f, 1.f,
             0.5f, -0.5f, -1.f,     0.2f, 0.3f, 0.9f, 1.f,
             0.0f,  0.5f, -1.f,     0.9f, 0.3f, 0.2f, 1.f
        };
        std::shared_ptr<Limnova::VertexBuffer> vertexBuffer;
        vertexBuffer.reset(Limnova::VertexBuffer::Create(vertices, sizeof(vertices)));
        vertexBuffer->SetLayout({
            { Limnova::ShaderDataType::Float3, "a_Position" },
            { Limnova::ShaderDataType::Float4, "a_Color" }
        });
        m_VertexArray->AddVertexBuffer(vertexBuffer);

        uint32_t indices[3] = { 0, 1, 2 };
        std::shared_ptr<Limnova::IndexBuffer> indexBuffer;
        indexBuffer.reset(Limnova::IndexBuffer::Create(indices, std::size(indices)));
        m_VertexArray->SetIndexBuffer(indexBuffer);

        /// Square
        m_SquareVA.reset(Limnova::VertexArray::Create());

        float squareVertices[(3 + 2) * 4] = {
            -0.5f, -0.5f, -1.f,   0.f, 0.f,
             0.5f, -0.5f, -1.f,   1.f, 0.f,
             0.5f,  0.5f, -1.f,   1.f, 1.f,
            -0.5f,  0.5f, -1.f,   0.f, 1.f
        };
        std::shared_ptr<Limnova::VertexBuffer> squareVB;
        squareVB.reset(Limnova::VertexBuffer::Create(squareVertices, sizeof(squareVertices)));
        squareVB->SetLayout({
            { Limnova::ShaderDataType::Float3, "a_Position" },
            { Limnova::ShaderDataType::Float2, "a_TexCoord" }
        });
        m_SquareVA->AddVertexBuffer(squareVB);

        uint32_t squareIndices[6] = { 0, 1, 2, 0, 2, 3 };
        std::shared_ptr<Limnova::IndexBuffer> squareIB;
        squareIB.reset(Limnova::IndexBuffer::Create(squareIndices, std::size(squareIndices)));
        m_SquareVA->SetIndexBuffer(squareIB);

        // Shaders
        m_Shader = Limnova::Shader::Create(ASSET_DIR"\\shaders\\AttrColor.glsl");
        m_Shader->BindUniformBuffer(Limnova::Renderer::GetCameraBufferId(), "CameraUniform");

        m_FlatColorShader = Limnova::Shader::Create(ASSET_DIR"\\shaders\\FlatColor.glsl");
        m_FlatColorShader->BindUniformBuffer(Limnova::Renderer::GetCameraBufferId(), "CameraUniform");

        m_TextureShader = Limnova::Shader::Create(ASSET_DIR"\\shaders\\texture.glsl");
        m_TextureShader->BindUniformBuffer(Limnova::Renderer::GetCameraBufferId(), "CameraUniform");
        m_TextureShader->Bind();
        std::dynamic_pointer_cast<Limnova::OpenGLShader>(m_TextureShader)->UploadUniformInt("u_Texture", 0);

        // Textures
        m_CheckerboardTexture = Limnova::Texture2D::Create(ASSET_DIR"\\textures\\testtex.png");
        m_TurretTexture = Limnova::Texture2D::Create(ASSET_DIR"\\textures\\turret.png");

        // Animation
        m_TrianglePosition = { 0.f, 0.f, 0.f };
        m_TriangleMoveSpeed = 1.f;

        m_SquareColor = { 0.2f, 0.3f, 0.9f };

        m_Time = std::chrono::steady_clock::now(); // TEMPORARY dT
    }


    void OnUpdate(Limnova::Timestep dT) override
    {
        // Animate camera
        auto [mouseX, mouseY] = Limnova::Input::GetMousePosition();
        float deltaMouseX = mouseX - m_MouseX;
        float deltaMouseY = mouseY - m_MouseY;
        m_MouseX = mouseX;
        m_MouseY = mouseY;
        if (m_MouseAimEnabled)
        {
            // Wrap azimuth around [0,360]
            m_CameraAzimuth -= m_MouseSensitivity * deltaMouseX;
            if (m_CameraAzimuth >= 360.f) m_CameraAzimuth -= 360.f;
            if (m_CameraAzimuth < 0.f) m_CameraAzimuth += 360.f;

            // Clamp elevation to [min,max] - prevents invalid UP vector in
            // projection matrix calculation
            m_CameraElevation += m_MouseSensitivity * deltaMouseY;
            m_CameraElevation = std::clamp(m_CameraElevation, m_MinElevation, m_MaxElevation);

            Limnova::Vector3 cameraAim = { 0.f, 0.f,-1.f }; // Default FORWARD aim (0,0,-1)
            cameraAim = Limnova::Rotate(cameraAim, { -1.f, 0.f, 0.f }, glm::radians(m_CameraElevation));
            cameraAim = Limnova::Rotate(cameraAim, m_CameraUp, glm::radians(m_CameraAzimuth));
            cameraAim.Normalize();
            m_TestCamera->SetAimDirection(cameraAim.glm_vec3());

            Limnova::Vector3 cameraMovement;
            Limnova::Vector3 cameraHorzLeft = m_CameraUp.Cross(cameraAim).Normalized();
            if (Limnova::Input::IsKeyPressed(LV_KEY_A))
            {
                cameraMovement += cameraHorzLeft;
            }
            else if (Limnova::Input::IsKeyPressed(LV_KEY_D))
            {
                cameraMovement -= cameraHorzLeft;
            }

            Limnova::Vector3 cameraHorzForward = cameraHorzLeft.Cross(m_CameraUp);
            if (Limnova::Input::IsKeyPressed(LV_KEY_W))
            {
                cameraMovement += cameraHorzForward;
            }
            else if (Limnova::Input::IsKeyPressed(LV_KEY_S))
            {
                cameraMovement -= cameraHorzForward;
            }

            cameraMovement.Normalize(); // Normalize horizontal movement

            if (Limnova::Input::IsKeyPressed(LV_KEY_SPACE))
            {
                cameraMovement.y += 1.f;
            }
            else if (Limnova::Input::IsKeyPressed(LV_KEY_LEFT_SHIFT))
            {
                cameraMovement.y -= 1.f;
            }

            m_CameraPos += dT * m_CameraMoveSpeed * cameraMovement;
            m_TestCamera->SetPosition(m_CameraPos.glm_vec3());

            // Triangle movement
            if (Limnova::Input::IsKeyPressed(LV_KEY_L))
            {
                m_TrianglePosition.x += m_TriangleMoveSpeed * dT;
            }
            else if (Limnova::Input::IsKeyPressed(LV_KEY_J))
            {
                m_TrianglePosition.x -= m_TriangleMoveSpeed * dT;
            }

            if (Limnova::Input::IsKeyPressed(LV_KEY_I))
            {
                m_TrianglePosition.y += m_TriangleMoveSpeed * dT;
            }
            else if (Limnova::Input::IsKeyPressed(LV_KEY_K))
            {
                m_TrianglePosition.y -= m_TriangleMoveSpeed * dT;
            }
        }

        // Rendering
        Limnova::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.f });
        Limnova::RenderCommand::Clear();

        Limnova::Renderer::BeginScene(m_TestCamera);

        m_FlatColorShader->Bind();
        std::dynamic_pointer_cast<Limnova::OpenGLShader>(m_FlatColorShader)->UploadUniformFloat3("u_Color", m_SquareColor);
        glm::mat4 squareTransform = glm::translate(glm::mat4(1.f), {-0.5f, 0.f, 0.f });
        Limnova::Renderer::Submit(m_FlatColorShader, m_SquareVA, squareTransform);

        glm::mat4 texSqTransform = glm::translate(glm::mat4(1.f), { 0.5f, 0.f, 0.f });
        m_CheckerboardTexture->Bind(0);
        Limnova::Renderer::Submit(m_TextureShader, m_SquareVA, texSqTransform);
        m_TurretTexture->Bind(0);
        Limnova::Renderer::Submit(m_TextureShader, m_SquareVA, texSqTransform);

        glm::mat4 triangleTransform = glm::translate(glm::mat4(0.5f), m_TrianglePosition);
        Limnova::Renderer::Submit(m_Shader, m_VertexArray, triangleTransform);

        Limnova::Renderer::EndScene();
    }


    void OnImGuiRender() override
    {
        ImGui::Begin("Test");
        ImGui::ColorEdit3("Square Color", glm::value_ptr(m_SquareColor));
        ImGui::End();
    }


    void OnEvent(Limnova::Event& event) override
    {
        Limnova::EventDispatcher dispatcher(event);
        dispatcher.Dispatch<Limnova::MouseButtonPressedEvent>(LV_BIND_EVENT_FN(DevLayer::OnMouseButtonPressedEvent));
    }


    bool OnMouseButtonPressedEvent(Limnova::MouseButtonPressedEvent& event)
    {
        if (event.GetMouseButton() == LV_MOUSE_BUTTON_RIGHT)
        {
            if (m_MouseAimEnabled)
            {
                m_MouseAimEnabled = false;
                m_TestCamera->DisableMouseAim();
            }
            else
            {
                m_MouseAimEnabled = true;
                m_TestCamera->EnableMouseAim();
            }
        }
        return false;
    }


    Limnova::Ref<Limnova::VertexArray> m_VertexArray;
    Limnova::Ref<Limnova::Shader> m_Shader;
    Limnova::Ref<Limnova::VertexArray> m_SquareVA;
    Limnova::Ref<Limnova::Shader> m_FlatColorShader;
    Limnova::Ref<Limnova::Texture2D> m_CheckerboardTexture;
    Limnova::Ref<Limnova::Texture2D> m_TurretTexture;
    Limnova::Ref<Limnova::Shader> m_TextureShader;

    std::chrono::steady_clock::time_point m_Time; // TEMPORARY dT

    // TEMPORARY camera animation
    std::shared_ptr<Limnova::PointCamera> m_TestCamera;
    Limnova::Vector3 m_CameraPos;
    Limnova::Vector3 m_CameraUp;
    float m_CameraMoveSpeed;
    bool m_MouseAimEnabled;
    float m_CameraElevation;
    float m_CameraAzimuth;
    float m_MouseX, m_MouseY;
    float m_MouseSensitivity;
    float m_MinElevation, m_MaxElevation;

    // TEMPORARY transform tests
    glm::vec3 m_TrianglePosition;
    float m_TriangleMoveSpeed;

    // TEMPORARY uniform tests
    glm::vec3 m_SquareColor;
};


class LIMNOVA_API DevApp : public Limnova::Application
{
public:
    DevApp()
    {
        PushLayer(new DevLayer());
    }

    ~DevApp()
    {
    }
};


Limnova::Application* Limnova::CreateApplication()
{
    return new DevApp();
}
