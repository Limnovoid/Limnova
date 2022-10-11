#pragma once

#ifdef LV_PLATFORM_WINDOWS

extern Limnova::Application* Limnova::CreateApplication();

int main(int argc, char** arvc)
{
	Limnova::Log::Init();
	LV_CORE_TRACE("Core log trace");
	LV_CORE_INFO("Core log info");
	LV_CORE_WARN("Core log warn");
	LV_CORE_ERROR("Core log error");
	LV_CORE_CRITICAL("Core log critical");

	LV_TRACE("Client log trace");
	LV_INFO("Client log info");
	LV_WARN("Client log warn");
	LV_ERROR("Client log error");
	LV_CRITICAL("Client log critical");	

	auto app = Limnova::CreateApplication();
	app->run();
	delete app;

	return 0;
}

#endif