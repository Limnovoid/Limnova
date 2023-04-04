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

        uint32_t GetColorAttachmentRendererId(uint32_t index = 0) const override { return m_ColorAttachments[index]; }


        void ClearAttachment(uint32_t attachmentIndex, int clearValue) override;

        int ReadPixel(uint32_t x, uint32_t y, uint32_t attachmentIndex = 0) const override;
    private:
        void Reset();
    private:
        uint32_t m_RendererId = 0;
        FramebufferSpecification m_Specification;

        std::vector<FramebufferTextureSpecification> m_ColorAttachmentSpecifications;
        FramebufferTextureSpecification m_DepthAttachmentSpecification;

        std::vector<uint32_t> m_ColorAttachments;
        uint32_t m_DepthAttachment = 0;
    };

}
