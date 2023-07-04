#pragma once


#ifdef LV_PLATFORM_WINDOWS

extern Limnova::Application* Limnova::CreateApplication(ApplicationCommandLineArgs args);

int main(int argc, char** argv)
{
    Limnova::Log::Init();

    LV_PROFILE_BEGIN_SESSION("Startup", LV_DIR"/Profiling/Limnova-Profile-Startup.json");
    auto app = Limnova::CreateApplication({ argc, argv });
    LV_PROFILE_END_SESSION();

    LV_PROFILE_BEGIN_SESSION("Runtime", LV_DIR"/Profiling/Limnova-Profile-Runtime.json");
    app->Run();
    LV_PROFILE_END_SESSION();

    LV_PROFILE_BEGIN_SESSION("Shutdown", LV_DIR"/Profiling/Limnova-Profile-Shutdown.json");
    delete app;
    LV_PROFILE_END_SESSION();

    return 0;
}

#endif
