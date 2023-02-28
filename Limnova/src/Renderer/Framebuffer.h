#pragma once


namespace Limnova
{

    struct FramebufferSpecification
    {
        uint32_t Width, Height;

        bool SwapChainTarget = false;
    };


    class Framebuffer
    {
    public:
        virtual ~Framebuffer() {}

        static Ref<Framebuffer> Create(const FramebufferSpecification& spec);

        virtual const FramebufferSpecification& GetSpecification() const = 0;

        virtual void Resize(const uint32_t width, const uint32_t height) = 0;

        virtual void Bind() = 0;
        virtual void Unbind() = 0;

        virtual uint32_t GetColorAttachmentRendererId() const = 0;
    };

}
