#pragma once

#include "Core.h"
#include "Events/Event.h"
#include "Window.h"
#include "LayerStack.h"
#include "Events/ApplicationEvent.h"


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
		std::unique_ptr<Window> m_Window;
		bool m_Running = true;
		LayerStack m_LayerStack;

		bool OnWindowClose(WindowCloseEvent& e);
	private:
		static Application* s_Instance;
	};

	// To be defined in CLIENT.
	Application* CreateApplication();

}