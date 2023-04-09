#include "OpenGLBuffer.h"

#include <glad/glad.h>


namespace Limnova
{
    // VertexBuffer ////////////////////////////////////////////////////////////

    OpenGLVertexBuffer::OpenGLVertexBuffer(uint32_t size)
    {
        LV_PROFILE_FUNCTION();

        glCreateBuffers(1, &m_RendererId);
        glBindBuffer(GL_ARRAY_BUFFER, m_RendererId);
        glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
    }


    OpenGLVertexBuffer::OpenGLVertexBuffer(float* vertices, uint32_t size)
    {
        LV_PROFILE_FUNCTION();

        glCreateBuffers(1, &m_RendererId);
        glBindBuffer(GL_ARRAY_BUFFER, m_RendererId);
        glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
    }


    OpenGLVertexBuffer::~OpenGLVertexBuffer()
    {
        LV_PROFILE_FUNCTION();

        glDeleteBuffers(1, &m_RendererId);
    }


    void OpenGLVertexBuffer::SetData(const void* data, uint32_t size)
    {
        LV_PROFILE_FUNCTION();

        glBindBuffer(GL_ARRAY_BUFFER, m_RendererId);
        glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
    }


    void OpenGLVertexBuffer::Bind() const
    {
        LV_PROFILE_FUNCTION();

        glBindBuffer(GL_ARRAY_BUFFER, m_RendererId);
    }


    void OpenGLVertexBuffer::Unbind() const
    {
        LV_PROFILE_FUNCTION();

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }


    // IndexBuffer /////////////////////////////////////////////////////////////

    OpenGLIndexBuffer::OpenGLIndexBuffer(uint32_t* indices, uint32_t count)
        : m_Count(count)
    {
        LV_PROFILE_FUNCTION();

        glCreateBuffers(1, &m_RendererId);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererId);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(uint32_t), indices, GL_STATIC_DRAW);
    }


    OpenGLIndexBuffer::~OpenGLIndexBuffer()
    {
        LV_PROFILE_FUNCTION();

        glDeleteBuffers(1, &m_RendererId);
    }


    void OpenGLIndexBuffer::Bind() const
    {
        LV_PROFILE_FUNCTION();

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererId);
    }


    void OpenGLIndexBuffer::Unbind() const
    {
        LV_PROFILE_FUNCTION();

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }


    // UniformBuffer ///////////////////////////////////////////////////////////

    OpenGLUniformBuffer::OpenGLUniformBuffer(uint32_t binding, uint32_t size)
        : m_Size(size)
    {
        LV_PROFILE_FUNCTION();

        glCreateBuffers(1, &m_RendererId);
        glNamedBufferData(m_RendererId, size, nullptr, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, binding, m_RendererId);
    }


    OpenGLUniformBuffer::~OpenGLUniformBuffer()
    {
        LV_PROFILE_FUNCTION();

        glDeleteBuffers(1, &m_RendererId);
    }


    void OpenGLUniformBuffer::Bind() const
    {
        LV_PROFILE_FUNCTION();

        glBindBuffer(GL_UNIFORM_BUFFER, m_RendererId);
    }


    void OpenGLUniformBuffer::Unbind() const
    {
        LV_PROFILE_FUNCTION();

        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }


    void OpenGLUniformBuffer::UpdateData(void* data, uint32_t offset, uint32_t size)
    {
        LV_PROFILE_FUNCTION();

        LV_CORE_ASSERT(m_Size == size, "UpdateData was passed a data size which does not match the buffer size!");

        glNamedBufferSubData(m_RendererId, offset, size, data);
    }

}
