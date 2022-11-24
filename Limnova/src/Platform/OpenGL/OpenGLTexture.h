#pragma once

#include "Renderer/Texture.h"


namespace Limnova
{

    class OpenGLTexture2D : public Texture2D
    {
    public:
        OpenGLTexture2D(const std::string& path);
        ~OpenGLTexture2D();

        uint32_t GetWidth() const override { return m_Width; };
        uint32_t GetHeight() const override { return m_Height; }

        void Bind(const uint32_t slot = 0) const override;
    private:
        std::string m_Path; // TODO : move to asset manager
        uint32_t m_RendererId;
        uint32_t m_Width, m_Height;
    };

}
