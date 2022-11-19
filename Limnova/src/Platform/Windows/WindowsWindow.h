#pragma once

#include "Window.h"
#include "Renderer/RenderingContext.h"

#include <GLFW/glfw3.h>


namespace Limnova
{

    class WindowsWindow : public Window
    {
    public:
        WindowsWindow(const WindowProps& props);
        virtual ~WindowsWindow();

        void OnUpdate() override;

        inline unsigned int GetWidth() const override { return m_Data.Width; }
        inline unsigned int GetHeight() const override { return m_Data.Height; }

        // Window attributes
        inline void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }
        void SetVSync(bool enabled) override;
        inline bool IsVSync() const override { return m_Data.VSync; }

        inline void SetClipboardText(void* userdata, const char* text) override { glfwSetClipboardString(m_Window, text); }
        inline const char* GetClipboardText(void* userdata) const override { return glfwGetClipboardString(m_Window); }

        inline void* GetNativeWindow() const override { return m_Window; }

        void DisableCursor() override;
        void EnableCursor() override;
        void SetRawMouseInput(bool useRawMouseInput) override;
    private:
        virtual void Init(const WindowProps& props);
        virtual void Shutdown();
    private:
        GLFWwindow* m_Window;
        RenderingContext* m_Context;

        struct WindowData
        {
            std::string Title;
            unsigned int Width, Height;
            bool VSync;

            EventCallbackFn EventCallback;
        };

        WindowData m_Data;
    };

}