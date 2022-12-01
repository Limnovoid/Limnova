#include "Dev2D.h"

#define ASSET_DIR "C:\\Programming\\source\\Limnova\\DevTool\\assets"


Dev2DLayer::Dev2DLayer()
    : Layer("Dev2D")
{
}


void Dev2DLayer::OnAttach()
{
    // Camera
    Limnova::Application& app = Limnova::Application::Get();
    m_CameraController = std::make_shared<Limnova::PerspectivePlanarCameraController>(
        Limnova::Vector3(0.f, 0.f, 2.f), Limnova::Vector3(0.f, 0.f, -1.f),
        (float)app.GetWindow().GetWidth() / (float)app.GetWindow().GetHeight(),
        0.1f, 100.f, glm::radians(60.f)
    );
    /*m_CameraController = std::make_shared<Limnova::OrthographicPlanarCameraController>(
        Limnova::Vector3(0.f, 0.f, 1.f), Limnova::Vector3(0.f, 0.f, -1.f),
        (float)app.GetWindow().GetWidth() / (float)app.GetWindow().GetHeight(),
        0.1f, 100.f
    );*/

    // Square
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
    m_ShaderLibrary.Load(ASSET_DIR"\\shaders\\Texture.lvglsl");
    auto textureShader = m_ShaderLibrary.Get("Texture");
    textureShader->BindUniformBuffer(Limnova::Renderer::GetSceneUniformBufferId(), "CameraUniform");
    textureShader->Bind();
    textureShader->SetInt("u_Texture", 0);

    // Textures
    m_Texture = Limnova::Texture2D::Create(ASSET_DIR"\\textures\\turret.png");
}


void Dev2DLayer::OnDetach()
{

}


void Dev2DLayer::OnUpdate(Limnova::Timestep dT)
{
    // Update
    m_CameraController->OnUpdate(dT);

    // Render
    Limnova::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.f });
    Limnova::RenderCommand::Clear();

    Limnova::Renderer2D::BeginScene(m_CameraController->GetCamera());

    Limnova::Renderer2D::DrawQuad({-1.f,-1.f }, { 2.f, 2.f }, m_SquareColor);

    auto textureShader = m_ShaderLibrary.Get("Texture");
    m_Texture->Bind(0);
    Limnova::Renderer::Submit(textureShader, m_SquareVA, glm::translate(glm::mat4(1.f), { 0.f, 0.f, 0.5f }));

    Limnova::Renderer2D::EndScene();
}


void Dev2DLayer::OnImGuiRender()
{
    ImGui::Begin("Scene");
    ImGui::ColorEdit4("Square Color", glm::value_ptr(m_SquareColor));
    ImGui::End();
}


void Dev2DLayer::OnEvent(Limnova::Event& e)
{
    m_CameraController->OnEvent(e);
}

