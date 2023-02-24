#pragma once

#include "Renderer/Texture.h"


namespace Limnova
{

    class OpenGLTexture2D : public Texture2D
    {
    public:
        OpenGLTexture2D(const uint32_t width, const uint32_t height);
        OpenGLTexture2D(const std::string& path, const WrapMode wrap);
        ~OpenGLTexture2D();

        uint32_t GetRendererId() const override { return m_RendererId; }
        uint32_t GetWidth() const override { return m_Width; };
        uint32_t GetHeight() const override { return m_Height; }

        void Bind(const uint32_t slot = 0) const override;

        void SetWrapMode(const WrapMode wrap) override;

        void SetData(void* data, uint32_t size) override;

        bool operator==(const Texture& other) const override
        {
            return m_RendererId == ((OpenGLTexture2D&)other).m_RendererId;
        }
    private:
        std::string m_Path; // TODO : move to asset manager
        uint32_t m_RendererId;
        uint32_t m_Width, m_Height;
        uint32_t m_InternalFormat;
        uint32_t m_UsageFormat;
    };

}
