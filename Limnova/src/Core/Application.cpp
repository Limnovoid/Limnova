#include "Application.h"

#include "Input.h"
#include "KeyCodes.h"

#include "Renderer/Renderer.h"
#include "Renderer/Renderer2D.h"


namespace Limnova
{
    Application* Application::s_Instance = nullptr;
    Application& Application::Get() { return *s_Instance; }


    Application::Application(const std::string& name, ApplicationCommandLineArgs args)
        : m_CommandLineArgs(args)
    {
        LV_PROFILE_FUNCTION();

        LV_CORE_ASSERT(!s_Instance, "Application already exists!");
        s_Instance = this;

        //m_Window = std::unique_ptr<Window>(Window::Create());
        m_Window = Window::Create(WindowProps(name));
        m_Window->SetEventCallback(LV_BIND_EVENT_FN(Application::OnEvent));
        m_Window->SetVSync(false);

        // TODO - build definition to select renderer ??? or leave initialisation up to user application ??
        Renderer::Init();
        Renderer2D::Init();

        m_ImGuiLayer = new ImGuiLayer();
        PushOverlay(m_ImGuiLayer);

        m_Time = std::chrono::steady_clock::now();
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

            // TODO : game time tracking

            auto newTime = std::chrono::steady_clock::now();
            std::chrono::duration<double> dTchrono = newTime - m_Time;
            Timestep dT = dTchrono.count();
            if (dT > Timestep::kDefaultTimestep) {
                dT = Timestep::kDefaultTimestep;
            }
            m_Time = newTime;

            // Update layers
            if (!m_Minimized)
            {
                LV_PROFILE_SCOPE("LayerStack Update");

                for (Layer* layer : m_LayerStack) {
                    layer->OnUpdate(dT);
                }
            }

            {
                LV_PROFILE_SCOPE("LayerStack OnImGuiRender");

                m_ImGuiLayer->Begin();
                for (Layer* layer : m_LayerStack) {
                    layer->OnImGuiRender();
                }
                m_ImGuiLayer->End();
            }

            m_Window->OnUpdate();
        }

        for (Layer* layer : m_LayerStack) {
            layer->OnDetach();
        }
    }


    void Application::Close()
    {
        LV_PROFILE_FUNCTION();

        m_Running = false;
    }


    void Application::OnEvent(Event& e)
    {
        LV_PROFILE_FUNCTION();

        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowCloseEvent>(LV_BIND_EVENT_FN(Application::OnWindowClose));
        dispatcher.Dispatch<WindowResizeEvent>(LV_BIND_EVENT_FN(Application::OnWindowResize));

        for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); it++) {
            if (e.Handled) break;
            (*it)->OnEvent(e);
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

        if (e.GetWidth() == 0 || e.GetHeight() == 0) {
            m_Minimized = true;
            return true;
        }
        m_Minimized = false;

        Renderer::OnWindowResize(e.GetWidth(), e.GetHeight());

        return false;
    }

}
