#include "OpenGLTexture.h"

#include <stb_image.h>

#include <glad/glad.h>


namespace Limnova
{

    OpenGLTexture2D::OpenGLTexture2D(const std::string& path)
        : m_Path(path)
    {
        int width, height, channels;
        stbi_set_flip_vertically_on_load(1);
        stbi_uc* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
        LV_CORE_ASSERT(data, "Failed to load image!");
        m_Width = width;
        m_Height = height;

        GLenum internalFormat = 0, usageFormat = 0;
        if (channels == 3)
        {
            internalFormat = GL_RGB8;
            usageFormat = GL_RGB;
        }
        else if (channels == 4)
        {
            internalFormat = GL_RGBA8;
            usageFormat = GL_RGBA;
        }
        LV_CORE_ASSERT(internalFormat & usageFormat, "Failed to load image: number of channels not supported!");

        glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererId);
        glTextureStorage2D(m_RendererId, 1, internalFormat, m_Width, m_Height);

        glTextureParameteri(m_RendererId, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(m_RendererId, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTextureSubImage2D(m_RendererId, 0, 0, 0, m_Width, m_Height, usageFormat, GL_UNSIGNED_BYTE, data);

        stbi_image_free(data);
    }


    OpenGLTexture2D::~OpenGLTexture2D()
    {
        glDeleteTextures(1, &m_RendererId);
    }


    void OpenGLTexture2D::Bind(const uint32_t slot) const
    {
        glBindTextureUnit(slot, m_RendererId);
    }

}
