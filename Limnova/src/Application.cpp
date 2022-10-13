#include "Application.h"


#include "Log.h"
#include "Events/ApplicationEvent.h"

namespace Limnova
{

	Application::Application()
	{
	}//Application()

	Application::~Application()
	{
	}//~Application()

	void Application::run()
	{
		WindowResizeEvent e(1920, 1080);
		LV_TRACE(e);

		while (true);
	}//run()

}