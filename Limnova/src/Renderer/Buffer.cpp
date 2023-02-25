#include "Buffer.h"

#include "Renderer.h"
#include "Platform/OpenGL/OpenGLBuffer.h"


namespace Limnova
{

    // VertexBuffer ////////////////////////////////////////////////////////////

    Ref<VertexBuffer> VertexBuffer::Create(uint32_t size)
    {
        switch (Renderer::GetAPI())
        {
        case RendererAPI::API::None: LV_CORE_ASSERT(false, "RendererAPI::None is not supported!"); return nullptr;
        case RendererAPI::API::OpenGL: return CreateRef<OpenGLVertexBuffer>(size);
        }
        LV_CORE_ASSERT(false, "Renderer::GetAPI() returned unknown RendererAPI!");
        return nullptr;
    }


    Ref<VertexBuffer> VertexBuffer::Create(float* vertices, uint32_t size)
    {
        switch (Renderer::GetAPI())
        {
            case RendererAPI::API::None: LV_CORE_ASSERT(false, "RendererAPI::None is not supported!"); return nullptr;
            case RendererAPI::API::OpenGL: return CreateRef<OpenGLVertexBuffer>(vertices, size);
        }
        LV_CORE_ASSERT(false, "Renderer::GetAPI() returned unknown RendererAPI!");
        return nullptr;
    }


    // IndexBuffer /////////////////////////////////////////////////////////////

    Ref<IndexBuffer> IndexBuffer::Create(uint32_t* indices, uint32_t count)
    {
        switch (Renderer::GetAPI())
        {
            case RendererAPI::API::None: LV_CORE_ASSERT(false, "RendererAPI::None is not supported!"); return nullptr;
            case RendererAPI::API::OpenGL: return CreateRef<OpenGLIndexBuffer>(indices, count);
        }
        LV_CORE_ASSERT(false, "Renderer::GetAPI() returned unknown RendererAPI!");
        return nullptr;
    }


    // UniformBuffer ///////////////////////////////////////////////////////////

    Ref<UniformBuffer> UniformBuffer::Create(void* data, uint32_t size)
    {
        switch (Renderer::GetAPI())
        {
            case RendererAPI::API::None: LV_CORE_ASSERT(false, "RendererAPI::None is not supported!"); return nullptr;
            case RendererAPI::API::OpenGL: return CreateRef<OpenGLUniformBuffer>(data, size);
        }
        LV_CORE_ASSERT(false, "Renderer::GetAPI() returned unknown RendererAPI!");
        return nullptr;
    }


    // BufferElement ///////////////////////////////////////////////////////////

    uint32_t BufferElement::GetComponentCount() const
    {
        switch (Type)
        {
            case ShaderDataType::Float:     return 1;
            case ShaderDataType::Float2:    return 2;
            case ShaderDataType::Float3:    return 3;
            case ShaderDataType::Float4:    return 4;
            case ShaderDataType::Int:       return 1;
            case ShaderDataType::Int2:      return 2;
            case ShaderDataType::Int3:      return 3;
            case ShaderDataType::Int4:      return 4;
            case ShaderDataType::Mat3:      return 3 * 3;
            case ShaderDataType::Mat4:      return 4 * 4;
            case ShaderDataType::Bool:      return 1;
        }
        LV_CORE_ASSERT(false, "GetElementCount() was called on a BufferElement with an unknown ShaderDataType!");
        return 0;
    }


    // BufferLayout ////////////////////////////////////////////////////////////

    void BufferLayout::CalculateOffsetsAndStride()
    {
        m_Stride = 0;
        for (auto& element : m_Elements)
        {
            element.Offset = m_Stride;
            m_Stride += element.Size;
        }
    }

}
