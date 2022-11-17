#pragma once

#include "Core.h"
#include "Window.h"
#include "LayerStack.h"
#include "Events/Event.h"
#include "Events/ApplicationEvent.h"
#include "ImGui/ImGuiLayer.h"
#include "Renderer/Renderer.h"
#include "Renderer/Buffer.h"
#include "Renderer/Shader.h"
#include "Renderer/VertexArray.h"


namespace Limnova
{

    class LIMNOVA_API Application
    {
    public:
        Application();
        virtual ~Application();

        void Run();

        void OnEvent(Event& e);

        void PushLayer(Layer* layer);
        void PushOverlay(Layer* overlay);

        inline Window& GetWindow() { return *m_Window; }

        static Application& Get();
    private:
        bool OnWindowClose(WindowCloseEvent& e);

        ImGuiLayer* m_ImGuiLayer;
        std::unique_ptr<Window> m_Window;
        bool m_Running = true;
        LayerStack m_LayerStack;

        Renderer::CameraData m_Camera;
    private:
        static Application* s_Instance;
    };

    // To be defined in CLIENT.
    Application* CreateApplication();

}