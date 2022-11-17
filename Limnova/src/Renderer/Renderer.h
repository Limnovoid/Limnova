#pragma once

#include "RenderCommand.h"


namespace Limnova
{

    class Renderer
    {
    public:
        struct CameraData
        {
            glm::mat4 View;
            glm::mat4 Proj;
            glm::mat4 ViewProj;
        };

        static void BeginScene(const CameraData* camera);
        static void EndScene();

        static void Submit(const std::shared_ptr<VertexArray>& vertexArray);

        inline static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }
        
        inline static void InitCamera(const CameraData* camera) { m_CameraUniformBuffer.reset(UniformBuffer::Create((void*)camera, sizeof(CameraData))); }
        inline static const uint32_t GetCameraBufferId() { return m_CameraUniformBuffer->GetRendererId(); }
    private:
        static std::unique_ptr<UniformBuffer> m_CameraUniformBuffer;
    };

}
