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

        void Bind() override;
        void Unbind() override;

        uint32_t GetColorAttachmentRendererId() const override { return m_ColorAttachment; }
    private:
        uint32_t m_RendererId;
        uint32_t m_ColorAttachment, m_DepthAttachment;
        FramebufferSpecification m_Specification;
    private:
        void Reset();
    };

}
