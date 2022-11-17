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
        LV_CORE_ASSERT(!s_Instance, "Application already exists!");
        s_Instance = this;

        //m_Window = std::unique_ptr<Window>(Window::Create());
        m_Window.reset(Window::Create());
        m_Window->SetEventCallback(LV_BIND_EVENT_FN(Application::OnEvent));

        m_ImGuiLayer = new ImGuiLayer();
        PushOverlay(m_ImGuiLayer);


        // TEMPORARY RENDERING
        glm::vec3 pos = glm::vec3(0.f, 0.f, 1.f);
        glm::vec3 aim = glm::normalize(glm::vec3(1.f, 0.f, -1.f));
        glm::vec3 up = glm::vec3(0.f, 1.f, 0.f);
        m_Camera.View = glm::lookAtRH(pos, pos + aim, up);

        float fov = glm::radians(60.f); // degrees
        float aspect = (float)m_Window->GetWidth() / (float)m_Window->GetHeight();
        float nearPlane = 0.1f;
        float farPlane = 10.f;
        m_Camera.Proj = glm::perspectiveRH_ZO(fov, aspect, nearPlane, farPlane);
        //m_Camera.Proj[1, 1] *= -1.f; // Invert image across y-axis.

        m_Camera.ViewProj = m_Camera.Proj * m_Camera.View;

        Renderer::InitCamera(&m_Camera);
        // TEMPORARY RENDERING
    }

    Application::~Application()
    {
    }


    void Application::PushLayer(Layer* layer)
    {
        m_LayerStack.PushLayer(layer);
        layer->OnAttach();
    }


    void Application::PushOverlay(Layer* overlay)
    {
        m_LayerStack.PushOverlay(overlay);
        overlay->OnAttach();
    }


    void Application::Run()
    {
        while (m_Running)
        {
            Limnova::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.f });
            Limnova::RenderCommand::Clear();

            Limnova::Renderer::BeginScene(&m_Camera);
            // Update layers
            for (Layer* layer : m_LayerStack)
            {
                layer->OnUpdate(); // Submit render commands here
            }
            Limnova::Renderer::EndScene();

            m_ImGuiLayer->Begin();
            for (Layer* layer : m_LayerStack)
            {
                layer->OnImGuiRender();
            }
            m_ImGuiLayer->End();

            m_Window->OnUpdate();
        }
    }


    void Application::OnEvent(Event& e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowCloseEvent>(LV_BIND_EVENT_FN(Application::OnWindowClose));


        for (auto it = m_LayerStack.end(); it != m_LayerStack.begin(); )
        {
            (*--it)->OnEvent(e);
            if (e.Handled)
            {
                break;
            }
        }
    }


    bool Application::OnWindowClose(WindowCloseEvent& e)
    {
        m_Running = false;
        return true;
    }

}