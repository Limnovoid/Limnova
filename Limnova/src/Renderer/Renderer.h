#pragma once

#include "RenderCommand.h"
#include "Camera.h"
#include "PointCamera.h"


namespace Limnova
{

    class Renderer
    {
    public:
        static void BeginScene(const std::shared_ptr<Camera>& camera);
        static void EndScene();

        static void Submit(const std::shared_ptr<VertexArray>& vertexArray);

        inline static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }
        
        inline static void InitCameraBuffer(const Camera::BufferData* camera) { m_CameraUniformBuffer.reset(UniformBuffer::Create((void*)camera, sizeof(Camera::BufferData))); }
        inline static const uint32_t GetCameraBufferId() { return m_CameraUniformBuffer->GetRendererId(); }
    private:
        static std::unique_ptr<UniformBuffer> m_CameraUniformBuffer;
    };

}
