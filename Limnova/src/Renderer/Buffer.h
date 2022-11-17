#pragma once


namespace Limnova
{

    enum class ShaderDataType
    {
        None = 0,
        Float, Float2, Float3, Float4,
        Int, Int2, Int3, Int4,
        Mat3, Mat4,
        Bool
    };


    static uint32_t ShaderDataTypeSize(ShaderDataType type)
    {
        switch (type)
        {
        case ShaderDataType::Float:     return 4;
        case ShaderDataType::Float2:    return 4 * 2;
        case ShaderDataType::Float3:    return 4 * 3;
        case ShaderDataType::Float4:    return 4 * 4;
        case ShaderDataType::Int:       return 4;
        case ShaderDataType::Int2:      return 4 * 2;
        case ShaderDataType::Int3:      return 4 * 3;
        case ShaderDataType::Int4:      return 4 * 4;
        case ShaderDataType::Mat3:      return 4 * 3 * 3;
        case ShaderDataType::Mat4:      return 4 * 4 * 4;
        case ShaderDataType::Bool:      return 1;
        }
        LV_CORE_ASSERT(false, "ShaderDataTypeSize() was passed an unknown ShaderDataType!");
        return 0;
    }


    struct BufferElement
    {
        ShaderDataType Type;
        std::string Name;
        uint32_t Size;
        uint32_t Offset;
        bool Normalized;

        BufferElement() : Type(ShaderDataType::None), Name(""), Size(0), Offset(0), Normalized(false)
        {
        }
        BufferElement(ShaderDataType type, const std::string& name, const bool normalized = false)
            : Type(type), Name(name), Size(ShaderDataTypeSize(type)), Offset(0), Normalized(normalized)
        {
        }

        uint32_t GetComponentCount() const;
    };


    class BufferLayout
    {
    public:
        BufferLayout() : m_Stride(0)
        {
        }
        BufferLayout(const std::initializer_list<BufferElement>& elements)
            : m_Elements(elements), m_Stride(0)
        {
            CalculateOffsetsAndStride();
        }

        inline uint32_t GetStride() const { return m_Stride; }
        inline const std::vector<BufferElement>& GetElements() const { return m_Elements; };

        std::vector<BufferElement>::iterator begin() { return m_Elements.begin(); }
        std::vector<BufferElement>::iterator end() { return m_Elements.end(); }
        std::vector<BufferElement>::const_iterator begin() const { return m_Elements.begin(); }
        std::vector<BufferElement>::const_iterator end() const { return m_Elements.end(); }
    private:
        void CalculateOffsetsAndStride();
    private:
        std::vector<BufferElement> m_Elements;
        uint32_t m_Stride;
    };


    class VertexBuffer
    {
    public:
        static VertexBuffer* Create(float* vertices, uint32_t size);
        virtual ~VertexBuffer() {}

        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;

        virtual const BufferLayout& GetLayout() const = 0;
        virtual void SetLayout(const BufferLayout& layout) = 0;
    };


    class IndexBuffer
    {
    public:
        static IndexBuffer* Create(uint32_t* indices, uint32_t count);
        virtual ~IndexBuffer() {}

        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;

        virtual inline uint32_t GetCount() const = 0;
    };


    class UniformBuffer
    {
    public:
        static UniformBuffer* Create(void* data, uint32_t size);
        virtual ~UniformBuffer() {};

        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;

        virtual void UpdateData(void* data, uint32_t size) = 0;

        virtual const uint32_t GetRendererId() = 0;
    };

}
