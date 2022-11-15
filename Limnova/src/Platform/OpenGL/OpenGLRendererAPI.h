#pragma once

#include "Renderer/RendererAPI.h"


namespace Limnova
{

    class OpenGLRendererAPI : public RendererAPI
    {
    public:
        void SetClearColor(const glm::vec4& color) override;
        void Clear() override;

        void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray) override;
    };

}
