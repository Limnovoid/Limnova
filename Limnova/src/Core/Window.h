#pragma once

#include "Core.h"
#include "Events/Event.h"


namespace Limnova
{

    struct WindowProps
    {
        std::string Title;
        uint32_t Width;
        uint32_t Height;

        WindowProps(const std::string& title = "Limnova Engine",
            uint32_t width = 1600, uint32_t height = 900)
            : Title(title), Width(width), Height(height)
        {
        }
    };


    // Interface for desktop-based Window
    class LIMNOVA_API Window
    {
    public:
        virtual ~Window() {}

        static Scope<Window> Create(const WindowProps& props = WindowProps());

        virtual void OnUpdate() = 0;

        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;

        // Window attributes
        using EventCallbackFn = std::function<void(Event&)>;
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