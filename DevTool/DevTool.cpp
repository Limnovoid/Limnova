#include "Limnova.h"


class LIMNOVA_API DevLayer : public Limnova::Layer
{
public:
	DevLayer()
		: Layer("Development Layer")
	{
	}

	void OnUpdate() override
	{
	}

	void OnEvent(Limnova::Event& event) override
	{
		LV_TRACE("{0}", event);
	}
};


class LIMNOVA_API DevApp : public Limnova::Application
{
public:
	DevApp()
	{
		PushLayer(new DevLayer());
		PushOverlay(new Limnova::ImGuiLayer());
	}

	~DevApp()
	{
	}
};


Limnova::Application* Limnova::CreateApplication()
{
	return new DevApp();
}