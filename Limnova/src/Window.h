#pragma once

#include "Core.h"
#include "Events/Event.h"


namespace Limnova
{

    struct WindowProps
    {
        std::string Title;
        unsigned int Width;
        unsigned int Height;

        WindowProps(const std::string& title = "Limnova Engine",
            unsigned int width = 1280, unsigned int height = 720)
            : Title(title), Width(width), Height(height)
        {
        }
    };


    // Interface for desktop-based Window
    class LIMNOVA_API Window
    {
    public:
        using EventCallbackFn = std::function<void(Event&)>;

        static Window* Create(const WindowProps& props = WindowProps());
        virtual ~Window() {}

        virtual void OnUpdate() = 0;

        virtual unsigned int GetWidth() const = 0;
        virtual unsigned int GetHeight() const = 0;

        // Window attributes
        virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
        virtual void SetVSync(bool enabled) = 0;
        virtual bool IsVSync() const = 0;

        virtual void SetClipboardText(void* userdata, const char* text) = 0;
        virtual const char* GetClipboardText(void* userdata) const = 0;

        virtual void* GetNativeWindow() const = 0;

        virtual void DisableCursor() = 0;
        virtual void EnableCursor() = 0;
        virtual void SetRawMouseInput(bool useRawMouseInput) = 0;
    };

}