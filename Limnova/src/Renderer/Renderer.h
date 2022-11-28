#pragma once

#include "RenderCommand.h"
#include "Camera.h"
#include "Shader.h"


namespace Limnova
{

    class Renderer
    {
    public:
        static void Init();
        static void OnWindowResize(uint32_t width, uint32_t height);

        static void BeginScene(Camera& camera);
        static void EndScene();

        static void Submit(const Ref<Shader>& shader, const Ref<VertexArray>& vertexArray, const glm::mat4& transform);

        inline static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

        inline static const uint32_t GetCameraBufferId() { return m_CameraUniformBuffer->GetRendererId(); }
    private:
        static Ref<UniformBuffer> m_CameraUniformBuffer;
    };

}
