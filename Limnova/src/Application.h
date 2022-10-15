#pragma once

#include "Core.h"
#include "Window.h"
#include "Events/ApplicationEvent.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"


namespace Limnova
{

	class LIMNOVA_API Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();

		void OnEvent(Event& e);
	private:
		std::unique_ptr<Window> m_Window;
		bool m_Running = true;

		bool OnWindowClose(WindowCloseEvent& e);
	};//Application

	// To be defined in CLIENT.
	Application* CreateApplication();

}