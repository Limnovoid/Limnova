#pragma once

#include "RenderCommand.h"
#include "Camera.h"
#include "PointCamera.h"
#include "Shader.h"


namespace Limnova
{

    class Renderer
    {
    public:
        static void Init();

        static void BeginScene(const Ref<Camera>& camera);
        static void EndScene();

        static void Submit(const Ref<Shader>& shader, const Ref<VertexArray>& vertexArray, const glm::mat4& transform);

        inline static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

        inline static void InitCameraBuffer(const Ref<Camera>& camera) { m_CameraUniformBuffer.reset(UniformBuffer::Create((void*)camera->GetData(), sizeof(Camera::BufferData))); }
        inline static const uint32_t GetCameraBufferId() { return m_CameraUniformBuffer->GetRendererId(); }
    private:
        static Scope<UniformBuffer> m_CameraUniformBuffer;
    };

}
