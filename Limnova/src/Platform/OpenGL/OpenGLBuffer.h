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

        inline const BufferLayout& GetLayout() const override { return m_Layout; }
        inline void SetLayout(const BufferLayout& layout) override { m_Layout = layout; }
    private:
        uint32_t m_RendererId;
        BufferLayout m_Layout;
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


    // UniformBuffer ///////////////////////////////////////////////////////////
    class OpenGLUniformBuffer : public UniformBuffer
    {
    public:
        OpenGLUniformBuffer(void* data, uint32_t size);
        ~OpenGLUniformBuffer();

        void Bind() const override;
        void Unbind() const override;

        void UpdateData(void* data, uint32_t size) override;

        inline const uint32_t GetRendererId() override { return m_RendererId; }
    private:
        uint32_t m_RendererId;
        uint32_t m_Size;
    };

}
