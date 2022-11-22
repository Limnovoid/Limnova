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
        static void BeginScene(const std::shared_ptr<Camera>& camera);
        static void EndScene();

        static void Submit(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vertexArray, const glm::mat4& transform);

        inline static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

        inline static void InitCameraBuffer(const std::shared_ptr<Camera>& camera) { m_CameraUniformBuffer.reset(UniformBuffer::Create((void*)camera->GetData(), sizeof(Camera::BufferData))); }
        inline static const uint32_t GetCameraBufferId() { return m_CameraUniformBuffer->GetRendererId(); }
    private:
        static std::unique_ptr<UniformBuffer> m_CameraUniformBuffer;
    };

}
