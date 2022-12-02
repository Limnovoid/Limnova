#include "OpenGLTexture.h"

#include <stb_image.h>

#include <glad/glad.h>


namespace Limnova
{

    OpenGLTexture2D::OpenGLTexture2D(const uint32_t width, const uint32_t height)
        : m_Width(width), m_Height(height),
        m_InternalFormat(GL_RGBA8), m_UsageFormat(GL_RGBA)
    {
        LV_PROFILE_FUNCTION();

        glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererId);
        glTextureStorage2D(m_RendererId, 1, m_InternalFormat, m_Width, m_Height);

        // TODO : parameters set by user
        glTextureParameteri(m_RendererId, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(m_RendererId, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        SetWrapMode(Texture::WrapMode::Tile);
    }


    OpenGLTexture2D::OpenGLTexture2D(const std::string& path, const WrapMode wrap)
        : m_Path(path)
    {
        LV_PROFILE_FUNCTION();

        int width, height, channels;
        stbi_set_flip_vertically_on_load(1);
        stbi_uc* data;
        {
            LV_PROFILE_SCOPE("stbi_load - OpenGLTexture2D(const std::string&, const WrapMode)");
            data = stbi_load(path.c_str(), &width, &height, &channels, 0);
        }
        LV_CORE_ASSERT(data, "Failed to load image!");
        m_Width = width;
        m_Height = height;

        if (channels == 3)
        {
            m_InternalFormat = GL_RGB8;
            m_UsageFormat = GL_RGB;
        }
        else if (channels == 4)
        {
            m_InternalFormat = GL_RGBA8;
            m_UsageFormat = GL_RGBA;
        }
        LV_CORE_ASSERT(m_InternalFormat && m_UsageFormat, "Failed to load image: number of channels not supported!");

        glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererId);
        glTextureStorage2D(m_RendererId, 1, m_InternalFormat, m_Width, m_Height);

        // TODO : parameters set by user
        glTextureParameteri(m_RendererId, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(m_RendererId, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        SetWrapMode(wrap);

        glTextureSubImage2D(m_RendererId, 0, 0, 0, m_Width, m_Height, m_UsageFormat, GL_UNSIGNED_BYTE, data);

        stbi_image_free(data);
    }


    OpenGLTexture2D::~OpenGLTexture2D()
    {
        LV_PROFILE_FUNCTION();

        glDeleteTextures(1, &m_RendererId);
    }


    void OpenGLTexture2D::Bind(const uint32_t slot) const
    {
        LV_PROFILE_FUNCTION();

        glBindTextureUnit(slot, m_RendererId);
    }


    void OpenGLTexture2D::SetWrapMode(const WrapMode wrap)
    {
        LV_PROFILE_FUNCTION();

        GLint mode;
        switch (wrap)
        {
            case WrapMode::Tile:            mode = GL_REPEAT; break;
            case WrapMode::MirroredTile:    mode = GL_MIRRORED_REPEAT; break;
            case WrapMode::Clamp:           mode = GL_CLAMP_TO_EDGE; break;
        default: LV_CORE_ERROR("Invalid wrap mode!"); mode = GL_REPEAT;
        }
        glTextureParameteri(m_RendererId, GL_TEXTURE_WRAP_S, mode);
        glTextureParameteri(m_RendererId, GL_TEXTURE_WRAP_T, mode);
    }


    void OpenGLTexture2D::SetData(void* data, uint32_t size)
    {
        LV_PROFILE_FUNCTION();

        LV_CORE_ASSERT(size == (m_UsageFormat == GL_RGBA ? 4 : 3) * m_Width * m_Height, "Data size must equal size of texture!");
        glTextureSubImage2D(m_RendererId, 0, 0, 0, m_Width, m_Height, m_UsageFormat, GL_UNSIGNED_BYTE, data);
    }

}
