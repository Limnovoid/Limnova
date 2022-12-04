#include "Application.h"

#include "Input.h"
#include "KeyCodes.h"

#include "Renderer/Renderer.h"


namespace Limnova
{
    Application* Application::s_Instance = nullptr;
    Application& Application::Get() { return *s_Instance; }


    Application::Application()
    {
        LV_PROFILE_FUNCTION();

        LV_CORE_ASSERT(!s_Instance, "Application already exists!");
        s_Instance = this;

        //m_Window = std::unique_ptr<Window>(Window::Create());
        m_Window.reset(Window::Create());
        m_Window->SetEventCallback(LV_BIND_EVENT_FN(Application::OnEvent));
        m_Window->SetVSync(true);

        Renderer::Init();

        m_ImGuiLayer = new ImGuiLayer();
        PushOverlay(m_ImGuiLayer);
    }

    Application::~Application()
    {
        LV_PROFILE_FUNCTION();
    }


    void Application::PushLayer(Layer* layer)
    {
        LV_PROFILE_FUNCTION();

        m_LayerStack.PushLayer(layer);
        layer->OnAttach();
    }


    void Application::PushOverlay(Layer* overlay)
    {
        LV_PROFILE_FUNCTION();

        m_LayerStack.PushOverlay(overlay);
        overlay->OnAttach();
    }


    void Application::Run()
    {
        LV_PROFILE_FUNCTION();

        while (m_Running)
        {
            LV_PROFILE_SCOPE("RunLoop");

            // TODO : custom time class
            auto newTime = std::chrono::steady_clock::now();
            std::chrono::duration<double> dTchrono = newTime - m_Time;
            Timestep dT = dTchrono.count();
            m_Time = newTime;

            // Update layers
            if (!m_Minimized)
            {
                {
                    LV_PROFILE_SCOPE("LayerStack Update");

                    for (Layer* layer : m_LayerStack)
                    {
                        layer->OnUpdate(dT);
                    }
                }
            }

            {
                LV_PROFILE_SCOPE("LayerStack OnImGuiRender");

                m_ImGuiLayer->Begin();
                for (Layer* layer : m_LayerStack)
                {
                    layer->OnImGuiRender();
                }
                m_ImGuiLayer->End();
            }


            m_Window->OnUpdate();
        }
    }


    void Application::OnEvent(Event& e)
    {
        LV_PROFILE_FUNCTION();

        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowCloseEvent>(LV_BIND_EVENT_FN(Application::OnWindowClose));
        dispatcher.Dispatch<WindowResizeEvent>(LV_BIND_EVENT_FN(Application::OnWindowResize));

        for (auto it = m_LayerStack.end(); it != m_LayerStack.begin(); )
        {
            if (e.Handled)
            {
                break;
            }
            (*--it)->OnEvent(e);
        }
    }


    bool Application::OnWindowClose(WindowCloseEvent& e)
    {
        m_Running = false;
        return true;
    }


    bool Application::OnWindowResize(WindowResizeEvent& e)
    {
        LV_PROFILE_FUNCTION();

        if (e.GetWidth() == 0 || e.GetHeight() == 0)
        {
            m_Minimized = true;
            return true;
        }
        m_Minimized = false;

        Renderer::OnWindowResize(e.GetWidth(), e.GetHeight());

        return false;
    }

}
