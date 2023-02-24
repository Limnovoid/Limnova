#pragma once

#include "Core.h"
#include "Window.h"

#include "Events/Event.h"
#include "Events/ApplicationEvent.h"
#include "LayerStack.h"
#include "ImGui/ImGuiLayer.h"
#include <Core/Timestep.h>

#include <chrono> // TEMPORARY delta time


namespace Limnova
{

    class Application
    {
    public:
        Application();
        virtual ~Application();

        static Application& Get();

        inline Window& GetWindow() { return *m_Window; }

        void PushLayer(Layer* layer);
        void PushOverlay(Layer* overlay);

        void Run();
        void Close();

        void OnEvent(Event& e);
    private:
        bool OnWindowClose(WindowCloseEvent& e);
        bool OnWindowResize(WindowResizeEvent& e);

        std::unique_ptr<Window> m_Window;
        bool m_Running = true;
        bool m_Minimized = false;

        LayerStack m_LayerStack;
        ImGuiLayer* m_ImGuiLayer;

        std::chrono::steady_clock::time_point m_Time; // TEMPORARY delta time
    private:
        static Application* s_Instance;
    };

    // To be defined in CLIENT.
    Application* CreateApplication();

}