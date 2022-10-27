#include "Limnova.h"

#include "imgui/imgui.h"


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

	void OnImGuiRender() override
	{
		// TODO : HOMEWORK - solve these linking errors
		ImGui::Begin("Test");
		ImGui::Text("Hello from DevTool!");
		ImGui::End();
	}

	void OnEvent(Limnova::Event& event) override
	{
	}
};


class LIMNOVA_API DevApp : public Limnova::Application
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