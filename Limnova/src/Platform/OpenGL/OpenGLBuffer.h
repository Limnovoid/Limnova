#pragma once

#include "Renderer/Buffer.h"


namespace Limnova
{

    // VertexBuffer ////////////////////////////////////////////////////////////
    class OpenGLVertexBuffer : public VertexBuffer
    {
    public:
        OpenGLVertexBuffer(float* vertices, uint32_t size);
        ~OpenGLVertexBuffer();

        void Bind() const override;
        void Unbind() const override;
    private:
        uint32_t m_RendererId;
    };


    // IndexBuffer /////////////////////////////////////////////////////////////
    class OpenGLIndexBuffer : public IndexBuffer
    {
    public:
        OpenGLIndexBuffer(uint32_t* indices, uint32_t count);
        ~OpenGLIndexBuffer();

        void Bind() const override;
        void Unbind() const override;

        inline uint32_t GetCount() const override { return m_Count; }
    private:
        uint32_t m_RendererId;
        uint32_t m_Count;
    };

}
