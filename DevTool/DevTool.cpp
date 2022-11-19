#include <Limnova.h>

#include <imgui/imgui.h>
#include <chrono>


class LIMNOVA_API DevLayer : public Limnova::Layer
{
public:
    DevLayer()
        : Layer("DevLayer")
    {
        // Camera setup
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

        // Triangle
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

        std::string vertexSrc = R"(
            #version 450

            layout (std140) uniform CameraUniform
            {
                mat4 ViewProj;
                vec4 Position;
                vec4 AimDirection;
            } u_Camera;

            layout(location = 0) in vec3 a_Position;
            layout(location = 1) in vec4 a_Color;

            out vec3 v_Position;
            out vec4 v_Color;

            void main()
            {
                v_Position = a_Position;
                v_Color = a_Color;
                gl_Position = u_Camera.ViewProj * vec4(a_Position, 1.0);
            }
        )";
        std::string fragmentSrc = R"(
            #version 450

            layout(location = 0) out vec4 o_Color;

            in vec3 v_Position;
            in vec4 v_Color;

            void main()
            {
                o_Color = vec4(v_Position * 0.5 + 0.5, 1.0);
                o_Color = v_Color;
            }
        )";
        m_Shader.reset(Limnova::Shader::Create(vertexSrc, fragmentSrc));

        m_Shader->BindUniformBuffer(Limnova::Renderer::GetCameraBufferId(), "CameraUniform");

        // Square
        m_SquareVA.reset(Limnova::VertexArray::Create());

        float squareVertices[3 * 4] = {
            -0.75f, -0.75f, -1.f,
             0.75f, -0.75f, -1.f,
             0.75f,  0.75f, -1.f,
            -0.75f,  0.75f, -1.f
        };
        std::shared_ptr<Limnova::VertexBuffer> squareVB;
        squareVB.reset(Limnova::VertexBuffer::Create(squareVertices, sizeof(squareVertices)));
        squareVB->SetLayout({
            { Limnova::ShaderDataType::Float3, "a_Position" }
        });
        m_SquareVA->AddVertexBuffer(squareVB);

        uint32_t squareIndices[6] = { 0, 1, 2, 0, 2, 3 };
        std::shared_ptr<Limnova::IndexBuffer> squareIB;
        squareIB.reset(Limnova::IndexBuffer::Create(squareIndices, std::size(squareIndices)));
        m_SquareVA->SetIndexBuffer(squareIB);

        std::string blueVertexSrc = R"(
            #version 450

            layout (std140) uniform CameraUniform
            {
                mat4 ViewProj;
                vec4 Position;
                vec4 AimDirection;
            } u_Camera;

            layout(location = 0) in vec3 a_Position;

            out vec3 v_Position;

            void main()
            {
                v_Position = a_Position;
                gl_Position = u_Camera.ViewProj * vec4(a_Position, 1.0);
            }
        )";
        std::string blueFragmentSrc = R"(
            #version 450

            layout(location = 0) out vec4 o_Color;

            in vec3 v_Position;

            void main()
            {
                o_Color = vec4(0.2, 0.3, 0.9, 1.0);
            }
        )";
        m_BlueShader.reset(Limnova::Shader::Create(blueVertexSrc, blueFragmentSrc));

        m_BlueShader->BindUniformBuffer(Limnova::Renderer::GetCameraBufferId(), "CameraUniform");

        m_Time = std::chrono::steady_clock::now(); // TEMPORARY dT
    }


    void OnUpdate() override
    {
        // TEMPORARY dT
        auto time2 = std::chrono::steady_clock::now();
        float dt = (time2 - m_Time).count() / 1e9;
        m_Time = time2;

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

            m_CameraPos += dt * m_CameraMoveSpeed * cameraMovement;
            m_TestCamera->SetPosition(m_CameraPos.glm_vec3());
        }

        // Rendering
        Limnova::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.f });
        Limnova::RenderCommand::Clear();

        Limnova::Renderer::BeginScene(m_TestCamera);

        Limnova::Renderer::Submit(m_BlueShader, m_SquareVA);
        Limnova::Renderer::Submit(m_Shader, m_VertexArray);

        Limnova::Renderer::EndScene();
    }


    void OnImGuiRender() override
    {
        ImGui::Begin("Test");
        ImGui::Text("Hello from DevTool!");
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


    std::shared_ptr<Limnova::Shader> m_Shader;
    std::shared_ptr<Limnova::VertexArray> m_VertexArray;
    std::shared_ptr<Limnova::Shader> m_BlueShader;
    std::shared_ptr<Limnova::VertexArray> m_SquareVA;

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
    // TEMPORARY camera animation
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
