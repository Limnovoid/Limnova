#include "OpenGLVertexArray.h"

#include "glad/glad.h"


namespace Limnova
{

    static GLenum ShaderDataTypeToOpenGLBaseType(ShaderDataType type)
    {
        switch (type)
        {
            case ShaderDataType::Float:     return GL_FLOAT;
            case ShaderDataType::Float2:    return GL_FLOAT;
            case ShaderDataType::Float3:    return GL_FLOAT;
            case ShaderDataType::Float4:    return GL_FLOAT;
            case ShaderDataType::Int:       return GL_INT;
            case ShaderDataType::Int2:      return GL_INT;
            case ShaderDataType::Int3:      return GL_INT;
            case ShaderDataType::Int4:      return GL_INT;
            case ShaderDataType::Mat3:      return GL_FLOAT;
            case ShaderDataType::Mat4:      return GL_FLOAT;
            case ShaderDataType::Bool:      return GL_BOOL;
        }
        LV_CORE_ASSERT(false, "ShaderDataTypeToOpenGLBaseType() was passd an unknown ShaderDataType!");
        return 0;
    }


    OpenGLVertexArray::OpenGLVertexArray()
    {
        LV_PROFILE_FUNCTION();

        glCreateVertexArrays(1, &m_RendererId);
    }

    OpenGLVertexArray::~OpenGLVertexArray()
    {
        LV_PROFILE_FUNCTION();

        glDeleteVertexArrays(1, &m_RendererId);
    }


    void OpenGLVertexArray::Bind() const
    {
        LV_PROFILE_FUNCTION();

        glBindVertexArray(m_RendererId);
    }


    void OpenGLVertexArray::Unbind() const
    {
        LV_PROFILE_FUNCTION();

        glBindVertexArray(0);
    }


    void OpenGLVertexArray::AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer)
    {
        LV_PROFILE_FUNCTION();

        LV_CORE_ASSERT(vertexBuffer->GetLayout().GetElements().size(), "VertexBuffer has no layout!");

        glBindVertexArray(m_RendererId);
        vertexBuffer->Bind();

        uint32_t index = 0;
        const auto& layout = vertexBuffer->GetLayout();
        for (const auto& element : layout)
        {
            glEnableVertexAttribArray(index);
            glVertexAttribPointer(index,
                element.GetComponentCount(),
                ShaderDataTypeToOpenGLBaseType(element.Type),
                element.Normalized ? GL_TRUE : GL_FALSE,
                layout.GetStride(),
                (const void*)element.Offset);
            index++;
        }
        m_VertexBuffers.push_back(vertexBuffer);
    }


    void OpenGLVertexArray::SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer)
    {
        LV_PROFILE_FUNCTION();

        glBindVertexArray(m_RendererId);
        indexBuffer->Bind();

        m_IndexBuffer = indexBuffer;
    }

}
