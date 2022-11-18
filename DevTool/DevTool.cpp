#include <Limnova.h>

#include <imgui/imgui.h>
#include <chrono>


class LIMNOVA_API DevLayer : public Limnova::Layer
{
public:
    DevLayer()
        : Layer("DevLayer")
    {
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


        // TEMPORARY camera testing
        Limnova::Application& app = Limnova::Application::Get();

        glm::vec3 pos = glm::vec3(0.f, 0.f, 1.f);
        glm::vec3 aim = glm::normalize(glm::vec3(0.f, 0.f, -1.f));
        glm::vec3 up = glm::vec3(0.f, 1.f, 0.f);

        float fov = glm::radians(60.f);
        float aspect = (float)app.GetWindow().GetWidth() / (float)app.GetWindow().GetHeight();
        float nearPlane = 0.1f;
        float farPlane = 100.f;

        Limnova::PointCamera* defaultCamera = new Limnova::PointCamera(fov, aspect, nearPlane, farPlane);
        defaultCamera->SetPosition(pos);
        defaultCamera->SetAimDirection(aim);
        defaultCamera->SetUpDirection(up);
        m_TestCamera.reset(defaultCamera);

        app.SetActiveCamera(m_TestCamera);
        m_CameraAim = aim;
        m_CameraRotationSpeed = glm::radians(60.f); // 10RPM
        m_Time = std::chrono::steady_clock::now();
        // TEMPORARY camera testing
    }


    void OnUpdate() override
    {
        // Animate camera
        auto time2 = std::chrono::steady_clock::now();
        float dt = (time2 - m_Time).count() / 1e9;
        m_Time = time2;
        m_CameraAim = Limnova::Rotate(m_CameraAim, { 0.f, 1.f, 0.f }, dt * m_CameraRotationSpeed);
        m_TestCamera->SetAimDirection({m_CameraAim.x, m_CameraAim.y, m_CameraAim.z});

        m_BlueShader->Bind();
        Limnova::Renderer::Submit(m_SquareVA);

        m_Shader->Bind();
        Limnova::Renderer::Submit(m_VertexArray);
    }


    void OnImGuiRender() override
    {
        ImGui::Begin("Test");
        ImGui::Text("Hello from DevTool!");
        ImGui::End();
    }


    void OnEvent(Limnova::Event& event) override
    {
    }

    std::shared_ptr<Limnova::Shader> m_Shader;
    std::shared_ptr<Limnova::VertexArray> m_VertexArray;
    std::shared_ptr<Limnova::Shader> m_BlueShader;
    std::shared_ptr<Limnova::VertexArray> m_SquareVA;

    // TEMP camera animation
    std::shared_ptr<Limnova::PointCamera> m_TestCamera;
    Limnova::Vector3 m_CameraAim;
    float m_CameraRotationSpeed;
    std::chrono::steady_clock::time_point m_Time;
    // TEMP camera animation
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
