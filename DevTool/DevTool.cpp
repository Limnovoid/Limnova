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


Limnova::Application* Limnova::CreateApplication()
{
	return new DevApp();
}