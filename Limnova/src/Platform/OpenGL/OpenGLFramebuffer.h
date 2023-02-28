#pragma once

#include "Renderer/Framebuffer.h"


namespace Limnova
{

    class OpenGLFramebuffer : public Framebuffer
    {
    public:
        OpenGLFramebuffer(const FramebufferSpecification& spec);
        ~OpenGLFramebuffer();

        const FramebufferSpecification& GetSpecification() const override { return m_Specification; }

        void Resize(const uint32_t width, const uint32_t height) override;

        void Bind() override;
        void Unbind() override;

        uint32_t GetColorAttachmentRendererId() const override { return m_ColorAttachment; }
    private:
        uint32_t m_RendererId = 0;
        uint32_t m_ColorAttachment, m_DepthAttachment;
        FramebufferSpecification m_Specification;
    private:
        void Reset();
    };

}
