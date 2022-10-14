#include "Application.h"

#include "Events/ApplicationEvent.h"


namespace Limnova
{

	Application::Application()
	{
		m_Window = std::unique_ptr<Window>(Window::Create());
	}//Application()

	Application::~Application()
	{
	}//~Application()

	void Application::run()
	{
		while (m_Running)
		{
			m_Window->OnUpdate();
		}
	}//run()

}