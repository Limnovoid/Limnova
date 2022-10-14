#pragma once

#include "Core.h"
#include "Window.h"


namespace Limnova
{

	class LIMNOVA_API Application
	{
	public:
		Application();
		virtual ~Application();

		void run();
	private:
		std::unique_ptr<Window> m_Window;
		bool m_Running = true;
	};//Application

	// To be defined in CLIENT.
	Application* CreateApplication();

}