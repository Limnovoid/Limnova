#include "Limnova.h"


class DevApp : public Limnova::Application
{
public:
	DevApp()
	{

	}

	~DevApp()
	{

	}
};


#include <stdio.h>
Limnova::Application* Limnova::CreateApplication()
{
	printf("Creating Application.\n");
	return new Limnova::Application();
}