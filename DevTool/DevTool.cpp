#include "Limnova.h"


class DevLayer : public Limnova::Layer
{
public:
	DevLayer()
		: Layer("Development Layer")
	{
	}

	void OnUpdate() override
	{
		LV_INFO("DevLayer::Update");
	}

	void OnEvent(Limnova::Event& event) override
	{
		LV_TRACE("{0}", event);
	}
};


class DevApp : public Limnova::Application
{
public:
	DevApp()
	{
		PushLayer(new DevLayer());
	}

	~DevApp()
	{
	}
};


Limnova::Application* Limnova::CreateApplication()
{
	return new DevApp();
}