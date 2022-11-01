#pragma once

#include "Renderer/RenderingContext.h"


struct GLFWwindow;

namespace Limnova
{

    class OpenGLContext : public RenderingContext
    {
    public:
        OpenGLContext(GLFWwindow* windowHandle);

        void Init() override;
        void SwapBuffers() override;
    private:
        GLFWwindow* m_WindowHandle;
    };

}