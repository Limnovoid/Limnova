#pragma once

#include <filesystem>

extern "C" {
    typedef struct _MonoAssembly MonoAssembly;
    typedef struct _MonoDomain MonoDomain;
    typedef struct _MonoImage MonoImage;
    typedef struct _MonoClass MonoClass;
}


namespace Limnova
{

    class ScriptEngine
    {
    public:
        ScriptEngine() = delete;

        static void Init();
        static void Shutdown();

    private:
        static void InitMono();
        static void ShutdownMono();

        static MonoAssembly* LoadMonoAssembly(std::filesystem::path const& assemblyPath);

        static void PrintAssemblyTypes(MonoAssembly* assembly);
    private:
        struct ScriptEngineData
        {
            MonoDomain* RootDomain = nullptr;
            MonoDomain* AppDomain = nullptr;

            MonoAssembly* CoreAssembly = nullptr;
            MonoImage* CoreAssemblyImage = nullptr;
        };
        static ScriptEngineData* s_SEData;

    };

}
