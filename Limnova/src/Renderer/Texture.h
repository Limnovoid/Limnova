#pragma once


namespace Limnova
{
    class Texture
    {
    public:
        virtual ~Texture() {}

        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;

        virtual void Bind(const uint32_t slot = 0) const = 0;

        enum class WrapMode : uint32_t
        {
            Tile            = 0,
            MirroredTile    = 1,
            Clamp           = 2
        };
        virtual void SetWrapMode(const WrapMode wrap) = 0;

        virtual void SetData(void* data, uint32_t size) = 0;
    };


    class Texture2D : public Texture
    {
    public:
        virtual ~Texture2D() {}

        static Ref<Texture2D> Create(const uint32_t width, const uint32_t height);
        static Ref<Texture2D> Create(const std::string& path, const WrapMode wrap = WrapMode::Tile);
    };

}
