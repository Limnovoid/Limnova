#pragma once

#include "RendererAPI.h"


namespace Limnova
{

    class RenderCommand
    {
    public:
        inline static void SetClearColor(const glm::vec4& color) { s_RendererApi->SetClearColor(color); }
        inline static void Clear() { s_RendererApi->Clear(); }

        inline static void DrawIndexed(const Ref<VertexArray>& vertexArray)
        {
            s_RendererApi->DrawIndexed(vertexArray);
        }
    private:
        static RendererAPI* s_RendererApi;
    };

}
