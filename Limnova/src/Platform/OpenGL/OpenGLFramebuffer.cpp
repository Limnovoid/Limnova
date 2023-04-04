#include "OpenGLFramebuffer.h"

#include <glad/glad.h>




namespace Limnova
{

    static const uint32_t s_MaxFramebufferSize = 8192; // TODO - get from GPU capabilities


    static GLenum TextureTarget(bool multisampling)
    {
        return multisampling ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
    }


    static GLenum LvFBTextureFormatToGL(FramebufferTextureFormat format)
    {
        switch (format)
        {
        case FramebufferTextureFormat::RGBA8:   return GL_RGBA;
        case FramebufferTextureFormat::RINT:    return GL_RED_INTEGER;
        }

        LV_CORE_ASSERT(false, "");
        return 0;
    }


    static void CreateTextures(GLenum target, uint32_t* outId, uint32_t count)
    {
        glCreateTextures(target, count, outId);
    }


    static void BindTexture(GLenum target, uint32_t id)
    {
        glBindTexture(target, id);
    }


    static void AttachColorTexture(GLenum textureTarget, int index, uint32_t id, int samples, GLenum internalFormat, GLenum format, uint32_t width, uint32_t height)
    {
        if (samples > 1) {
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, internalFormat, width, height, GL_FALSE);
        }
        else
        {
            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, nullptr);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, textureTarget, id, 0);
    }


    static void AttachDepthTexture(GLenum textureTarget, uint32_t id, int samples, GLenum internalFormat, GLenum attachmentType, uint32_t width, uint32_t height)
    {
        if (samples > 1) {
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, internalFormat, width, height, GL_FALSE);
        }
        else
        {
            glTexStorage2D(GL_TEXTURE_2D, 1, internalFormat, width, height);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }

        glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, textureTarget, id, 0);
    }


    static bool IsDepthFormat(FramebufferTextureFormat format)
    {
        switch (format)
        {
        case FramebufferTextureFormat::DEPTH24STENCIL8: return true;
        }
        return false;
    }


    OpenGLFramebuffer::OpenGLFramebuffer(const FramebufferSpecification& spec)
        : m_Specification(spec)
    {
        for (auto attachmentSpec : m_Specification.Attachments.Specifications)
        {
            if (IsDepthFormat(attachmentSpec.TextureFormat)) {
                m_DepthAttachmentSpecification = attachmentSpec;
            }
            else {
                m_ColorAttachmentSpecifications.emplace_back(attachmentSpec);
            }
        }

        Reset();
    }


    OpenGLFramebuffer::~OpenGLFramebuffer()
    {
        glDeleteFramebuffers(1, &m_RendererId);
        glDeleteTextures(m_ColorAttachments.size(), m_ColorAttachments.data());
        glDeleteTextures(1, &m_DepthAttachment);
    }


    void OpenGLFramebuffer::Reset()
    {
        if (m_RendererId != 0)
        {
            // Delete if already created
            glDeleteFramebuffers(1, &m_RendererId);
            glDeleteTextures(m_ColorAttachments.size(), m_ColorAttachments.data());
            glDeleteTextures(1, &m_DepthAttachment);

            m_ColorAttachments.clear();
            m_DepthAttachment = 0;
        }

        glCreateFramebuffers(1, &m_RendererId);
        glBindFramebuffer(GL_FRAMEBUFFER, m_RendererId);


        // Attachments //

        GLenum target = TextureTarget(m_Specification.Samples > 1);

        if (m_ColorAttachmentSpecifications.size() > 0)
        {
            m_ColorAttachments.resize(m_ColorAttachmentSpecifications.size());
            CreateTextures(target, m_ColorAttachments.data(), m_ColorAttachments.size());

            for (size_t i = 0; i < m_ColorAttachments.size(); i++)
            {
                BindTexture(target, m_ColorAttachments[i]);

                GLenum internalFormat = GL_RGBA8;
                switch (m_ColorAttachmentSpecifications[i].TextureFormat)
                {
                case FramebufferTextureFormat::RGBA8:
                    /* internalFormat = GL_RGBA8 */
                    break;
                case FramebufferTextureFormat::RINT:
                    internalFormat = GL_R32I;
                    break;
                }
                GLenum format = LvFBTextureFormatToGL(m_ColorAttachmentSpecifications[i].TextureFormat);

                AttachColorTexture(target, i, m_ColorAttachments[i], m_Specification.Samples,
                    internalFormat, format, m_Specification.Width, m_Specification.Height);
            }
        }

        if (m_DepthAttachmentSpecification.TextureFormat != FramebufferTextureFormat::None)
        {

            CreateTextures(target, &m_DepthAttachment, 1);
            BindTexture(target, m_DepthAttachment);

            GLenum format = GL_DEPTH24_STENCIL8, type = GL_DEPTH_STENCIL_ATTACHMENT;
            switch (m_DepthAttachmentSpecification.TextureFormat)
            {
            case FramebufferTextureFormat::DEPTH24STENCIL8:
                // format = GL_DEPTH24_STENCIL8;
                // type = GL_DEPTH_STENCIL_ATTACHMENT;
                break;
            }
            AttachDepthTexture(target, m_DepthAttachment, m_Specification.Samples,
                format, type, m_Specification.Width, m_Specification.Height);
        }

        if (m_ColorAttachments.size() > 1)
        {
            LV_CORE_ASSERT(m_ColorAttachments.size() <= 4, "Only supports up to 4 color attachments!");
            GLenum buffers[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
            glDrawBuffers(m_ColorAttachments.size(), buffers);
        }
        else if (m_ColorAttachments.empty())
        {
            // Depth only
            glDrawBuffer(GL_NONE);
        }


#ifdef exclude
        glCreateTextures(GL_TEXTURE_2D, 1, &m_ColorAttachment);
        glBindTexture(GL_TEXTURE_2D, m_ColorAttachment);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Specification.Width, m_Specification.Height,
            0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorAttachment, 0);

        glCreateTextures(GL_TEXTURE_2D, 1, &m_DepthAttachment);
        glBindTexture(GL_TEXTURE_2D, m_DepthAttachment);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH24_STENCIL8, m_Specification.Width, m_Specification.Height);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_DepthAttachment, 0);
#endif

        LV_CORE_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer is incomplete!");

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }


    void OpenGLFramebuffer::Resize(const uint32_t width, const uint32_t height)
    {
        if (width == 0 || height == 0 || width > s_MaxFramebufferSize || height > s_MaxFramebufferSize)
        {
            LV_CORE_WARN("Attempted to resize framebuffer to invalid value: {0}, {1}", width, height);
            return;
        }

        m_Specification.Width = width;
        m_Specification.Height = height;

        Reset();
    }


    void OpenGLFramebuffer::Bind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_RendererId);
        glViewport(0, 0, m_Specification.Width, m_Specification.Height);
    }


    void OpenGLFramebuffer::Unbind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }


    int OpenGLFramebuffer::ReadPixel(uint32_t x, uint32_t y, uint32_t attachmentIndex) const
    {
        LV_CORE_ASSERT(attachmentIndex < m_ColorAttachments.size(), "Attachment index out of bounds!");

        glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentIndex);
        int pixelData;
        glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, &pixelData);
        return pixelData;
    }


    void OpenGLFramebuffer::ClearAttachment(uint32_t attachmentIndex, int clearValue)
    {
        GLenum format = LvFBTextureFormatToGL(
            m_ColorAttachmentSpecifications[attachmentIndex].TextureFormat);

        glClearTexImage(m_ColorAttachments[attachmentIndex], 0,
            format, GL_INT, &clearValue);
    }

}
