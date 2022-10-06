#pragma once

#ifdef LV_PLATFORM_WINDOWS

extern Limnova::Application* Limnova::CreateApplication();

int main(int argc, char** arvc)
{
	auto app = Limnova::CreateApplication();
	app->run();
	delete app;

	return 0;
}

#endif