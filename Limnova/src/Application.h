#pragma once

#include "Core.h"


namespace Limnova
{

	class LIMNOVA_API Application
	{

	public:
		Application();
		virtual ~Application();

		void run();

	};//Application

	// To be defined in CLIENT.
	Application* CreateApplication();

}