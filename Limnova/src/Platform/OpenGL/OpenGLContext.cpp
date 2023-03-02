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


    OpenGLContext::~OpenGLContext()
    {
    }


    void OpenGLContext::Init()
    {
        LV_PROFILE_FUNCTION();

        glfwMakeContextCurrent(m_WindowHandle);
        int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
        LV_CORE_ASSERT(status, "Failed to initialize Glad!");

        LV_CORE_INFO("OpenGL Info:"
        "\n - Vendor:   {0}"
        "\n - Renderer: {1}"
        "\n - Version:  {2}",
            glGetString(GL_VENDOR), glGetString(GL_RENDERER), glGetString(GL_VERSION)
        );

#ifdef LV_ENABLE_ASSERTS
        int versionMajor;
        int versionMinor;
        glGetIntegerv(GL_MAJOR_VERSION, &versionMajor);
        glGetIntegerv(GL_MINOR_VERSION, &versionMinor);
        LV_CORE_ASSERT(versionMajor > 4 || (versionMajor == 4 && versionMinor >= 5), "Limnova requires at least OpenGL version 4.5!");
#endif
    }


    void OpenGLContext::Shutdown()
    {
    }


    void OpenGLContext::SwapBuffers()
    {
        LV_PROFILE_FUNCTION();

        glfwSwapBuffers(m_WindowHandle);
    }

}