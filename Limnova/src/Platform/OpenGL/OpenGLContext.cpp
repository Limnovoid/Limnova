#include "OpenGLContext.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>


namespace Limnova
{

    OpenGLContext::OpenGLContext(GLFWwindow* windowHandle)
        : m_WindowHandle(windowHandle)
    {
        LV_CORE_ASSERT(m_WindowHandle, "Window handle is null!");
    }


    void OpenGLContext::Init()
    {
        glfwMakeContextCurrent(m_WindowHandle);
        int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
        LV_CORE_ASSERT(status, "Failed to initialize Glad!");

        LV_CORE_INFO("OpenGL Info:\n"
        "    Vendor:   {0}\n"
        "    Renderer: {1}\n"
        "    Version:  {2}",
            glGetString(GL_VENDOR), glGetString(GL_RENDERER), glGetString(GL_VERSION)
        );
    }


    void OpenGLContext::Shutdown()
    {
    }


    void OpenGLContext::SwapBuffers()
    {
        glfwSwapBuffers(m_WindowHandle);
    }

}