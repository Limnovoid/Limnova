#include "OpenGLBuffer.h"

#include <glad/glad.h>


namespace Limnova
{
    // VertexBuffer ////////////////////////////////////////////////////////////

    OpenGLVertexBuffer::OpenGLVertexBuffer(float* vertices, uint32_t size)
    {
        glCreateBuffers(1, &m_RendererId);
        glBindBuffer(GL_ARRAY_BUFFER, m_RendererId);
        glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
    }


    OpenGLVertexBuffer::~OpenGLVertexBuffer()
    {
        glDeleteBuffers(1, &m_RendererId);
    }


    void OpenGLVertexBuffer::Bind() const
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_RendererId);
    }


    void OpenGLVertexBuffer::Unbind() const
    {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }


    // IndexBuffer /////////////////////////////////////////////////////////////

    OpenGLIndexBuffer::OpenGLIndexBuffer(uint32_t* indices, uint32_t count)
        : m_Count(count)
    {
        glCreateBuffers(1, &m_RendererId);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererId);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(uint32_t), indices, GL_STATIC_DRAW);
    }


    OpenGLIndexBuffer::~OpenGLIndexBuffer()
    {
        glDeleteBuffers(1, &m_RendererId);
    }


    void OpenGLIndexBuffer::Bind() const
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererId);
    }


    void OpenGLIndexBuffer::Unbind() const
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }


    // UniformBuffer ///////////////////////////////////////////////////////////

    OpenGLUniformBuffer::OpenGLUniformBuffer(void* data, uint32_t size)
        : m_Size(size)
    {
        glCreateBuffers(1, &m_RendererId);
        glBindBuffer(GL_UNIFORM_BUFFER, m_RendererId);
        glBufferData(GL_UNIFORM_BUFFER, size, data, GL_DYNAMIC_DRAW);
    }


    OpenGLUniformBuffer::~OpenGLUniformBuffer()
    {
        glDeleteBuffers(1, &m_RendererId);
    }


    void OpenGLUniformBuffer::Bind() const
    {
        glBindBuffer(GL_UNIFORM_BUFFER, m_RendererId);
    }


    void OpenGLUniformBuffer::Unbind() const
    {
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }


    void OpenGLUniformBuffer::UpdateData(void* data, uint32_t size)
    {
        LV_CORE_ASSERT(m_Size == size, "UpdateData was passed a data size which does not match the buffer size!");

        glBindBuffer(GL_UNIFORM_BUFFER, m_RendererId);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, size, data);
    }

}
