#include <Limnova.h>
#include <Core/EntryPoint.h>

#include "Dev2D.h"
#include "Orbiters2D.h"
#include "TestLayer.h"

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
        m_CameraController = std::make_shared<Limnova::PerspectivePointCameraController>(
            Limnova::Vector3(0.f, 0.f, 2.f), Limnova::Vector3(0.f, 0.f,-1.f),
            (float)app.GetWindow().GetWidth() / (float)app.GetWindow().GetHeight(),
            0.1f, 100.f, glm::radians(60.f)
        );
        /*m_CameraController = std::make_shared<Limnova::OrthographicPointCameraController>(
            Limnova::Vector3(0.f, 0.f, 1.f), Limnova::Vector3(0.f, 0.f, -1.f),
            (float)app.GetWindow().GetWidth() / (float)app.GetWindow().GetHeight(),
            0.1f, 100.f
        );*/
        app.GetWindow().SetRawMouseInput(true);

        // Vertex arrays
        /// Triangle
        m_VertexArray = Limnova::VertexArray::Create();

        float vertices[3 * (3 + 4)] = {
            -0.5f, -0.5f,  0.f,     0.2f, 0.9f, 0.3f, 1.f,
             0.5f, -0.5f,  0.f,     0.2f, 0.3f, 0.9f, 1.f,
             0.0f,  0.5f,  0.f,     0.9f, 0.3f, 0.2f, 1.f
        };
        Limnova::Ref<Limnova::VertexBuffer> vertexBuffer = Limnova::VertexBuffer::Create(vertices, sizeof(vertices));
        vertexBuffer->SetLayout({
            { Limnova::ShaderDataType::Float3, "a_Position" },
            { Limnova::ShaderDataType::Float4, "a_Color" }
        });
        m_VertexArray->AddVertexBuffer(vertexBuffer);

        uint32_t indices[3] = { 0, 1, 2 };
        Limnova::Ref<Limnova::IndexBuffer> indexBuffer = Limnova::IndexBuffer::Create(indices, std::size(indices));
        m_VertexArray->SetIndexBuffer(indexBuffer);

        /// Square
        m_SquareVA = Limnova::VertexArray::Create();

        float squareVertices[(3 + 2) * 4] = {
            -0.5f, -0.5f,  0.f,   0.f, 0.f,
             0.5f, -0.5f,  0.f,   1.f, 0.f,
             0.5f,  0.5f,  0.f,   1.f, 1.f,
            -0.5f,  0.5f,  0.f,   0.f, 1.f
        };
        Limnova::Ref<Limnova::VertexBuffer> squareVB = Limnova::VertexBuffer::Create(squareVertices, sizeof(squareVertices));
        squareVB->SetLayout({
            { Limnova::ShaderDataType::Float3, "a_Position" },
            { Limnova::ShaderDataType::Float2, "a_TexCoord" }
        });
        m_SquareVA->AddVertexBuffer(squareVB);

        uint32_t squareIndices[6] = { 0, 1, 2, 0, 2, 3 };
        Limnova::Ref<Limnova::IndexBuffer> squareIB = Limnova::IndexBuffer::Create(squareIndices, std::size(squareIndices));
        m_SquareVA->SetIndexBuffer(squareIB);

        // Shaders
        m_Shader = Limnova::Shader::Create(ASSET_DIR"\\shaders\\AttrColor.lvglsl");
        m_Shader->BindUniformBuffer(Limnova::Renderer::GetSceneUniformBufferId(), "CameraUniform");

        m_FlatColorShader = Limnova::Shader::Create(ASSET_DIR"\\shaders\\FlatColor.lvglsl");
        m_FlatColorShader->BindUniformBuffer(Limnova::Renderer::GetSceneUniformBufferId(), "CameraUniform");

        m_ShaderLibrary.Load(ASSET_DIR"\\shaders\\Texture.lvglsl");
        auto textureShader = m_ShaderLibrary.Get("Texture");
        textureShader->BindUniformBuffer(Limnova::Renderer::GetSceneUniformBufferId(), "CameraUniform");
        textureShader->Bind();
        textureShader->SetInt("u_Texture", 0);

        // Textures
        m_CheckerboardTexture = Limnova::Texture2D::Create(ASSET_DIR"\\textures\\testtex.png");
        m_TurretTexture = Limnova::Texture2D::Create(ASSET_DIR"\\textures\\turret.png");

        // Animation
        m_TrianglePosition = { 0.f, 0.f, 0.f };
        m_TriangleMoveSpeed = 1.f;

        m_SquareColor = { 0.2f, 0.3f, 0.9f, 1.f };

        m_Time = std::chrono::steady_clock::now(); // TEMPORARY dT
    }


    void OnUpdate(Limnova::Timestep dT) override
    {
        // Update
        m_CameraController->OnUpdate(dT);

        if (m_CameraController->IsBeingControlled())
        {
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

        Limnova::Renderer::BeginScene(m_CameraController->GetCamera());

        m_FlatColorShader->Bind();
        m_FlatColorShader->SetVec4("u_Color", m_SquareColor);
        glm::mat4 squareTransform = glm::translate(glm::mat4(1.f), {-0.5f, 0.f, 0.f });
        Limnova::Renderer::Submit(m_FlatColorShader, m_SquareVA, squareTransform);

        auto textureShader = m_ShaderLibrary.Get("Texture");
        glm::mat4 texSqTransform = glm::translate(glm::mat4(1.f), { 0.5f, 0.f, 0.f });
        m_CheckerboardTexture->Bind(0);
        Limnova::Renderer::Submit(textureShader, m_SquareVA, texSqTransform);
        m_TurretTexture->Bind(0);
        Limnova::Renderer::Submit(textureShader, m_SquareVA, texSqTransform);

        glm::mat4 triangleTransform = glm::translate(glm::mat4(0.5f), m_TrianglePosition);
        Limnova::Renderer::Submit(m_Shader, m_VertexArray, triangleTransform);

        Limnova::Renderer::EndScene();
    }


    void OnEvent(Limnova::Event& e)
    {
        m_CameraController->OnEvent(e);
    }


    void OnImGuiRender() override
    {
        ImGui::Begin("Test");
        ImGui::ColorEdit4("Square Color", glm::value_ptr(m_SquareColor));
        ImGui::End();
    }


    Limnova::ShaderLibrary m_ShaderLibrary;

    Limnova::Ref<Limnova::VertexArray> m_VertexArray;
    Limnova::Ref<Limnova::Shader> m_Shader;
    Limnova::Ref<Limnova::VertexArray> m_SquareVA;
    Limnova::Ref<Limnova::Shader> m_FlatColorShader;
    Limnova::Ref<Limnova::Texture2D> m_CheckerboardTexture;
    Limnova::Ref<Limnova::Texture2D> m_TurretTexture;

    std::chrono::steady_clock::time_point m_Time; // TEMPORARY dT

    // TEMPORARY camera animation
    Limnova::Ref<Limnova::PerspectivePointCameraController> m_CameraController;
    //Limnova::Ref<Limnova::OrthographicPointCameraController> m_CameraController;

    // TEMPORARY transform tests
    glm::vec3 m_TrianglePosition;
    float m_TriangleMoveSpeed;

    // TEMPORARY uniform tests
    glm::vec4 m_SquareColor;
};


namespace LV = Limnova;

class LIMNOVA_API DevApp : public Limnova::Application
{
public:
    DevApp()
    {
        //PushLayer(new TestLayer());
        // 
        //PushLayer(new DevLayer());
        //PushLayer(new Dev2DLayer());
        PushLayer(new Orbiters2D());
    }

    ~DevApp()
    {
    }
};


Limnova::Application* Limnova::CreateApplication()
{
    return new DevApp();
}
