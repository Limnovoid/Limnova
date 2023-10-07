#include "ScriptEngine.h"

#include "ScriptLibrary.h"

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>

// path to ScriptCoreAssembly.dll
#define SCRIPT_CORE_ASSEMBLY_PATH_BASE LV_DIR"/LimnovaEditor/Resources/lib/Scripting/"
#ifdef LV_DEBUG
    #define SCRIPT_CORE_ASSEMBLY_CONFIG "Debug"
#elif LV_RELEASE
    #define SCRIPT_CORE_ASSEMBLY_CONFIG "Release"
#endif
#define LV_SCRIPT_CORE_ASSEMBLY_PATH SCRIPT_CORE_ASSEMBLY_PATH_BASE SCRIPT_CORE_ASSEMBLY_CONFIG "/LimnovaScriptCore.dll"
//

namespace Limnova
{
    namespace Utils
    {
        static char* ReadBytes(std::filesystem::path const& filepath, uint32_t * outSize)
        {
            std::ifstream stream(filepath, std::ios::binary | std::ios::ate);

            if (!stream)
            {
                // Failed to open the file
                return nullptr;
            }

            std::streampos end = stream.tellg();
            stream.seekg(0, std::ios::beg);
            uint32_t size = end - stream.tellg();

            if (size == 0)
            {
                // File is empty
                return nullptr;
            }

            char* buffer = new char[size];
            stream.read((char*)buffer, size);
            stream.close();

            *outSize = size;
            return buffer;
        }
    }


    ScriptEngine::ScriptEngineData* ScriptEngine::s_SEData = nullptr;


    void ScriptEngine::Init()
    {
        LV_CORE_ASSERT(s_SEData == nullptr, "ScriptEngine is already initialized!");
        s_SEData = new ScriptEngineData;
        InitMono();
        ScriptLibrary::RegisterAllFunctions();
        ScriptLibrary::RegisterAllScriptClasses(s_SEData->CoreAssemblyImage);


        // testing
        MonoObject* instance = ScriptLibrary::ScriptClass_Main::Instantiate(s_SEData->AppDomain);
        ScriptLibrary::ScriptClass_Main::InvokeMethod("PrintMessage", instance);
    }

    void ScriptEngine::Shutdown()
    {
        ShutdownMono();
        delete s_SEData;
        s_SEData = nullptr;
    }


    void ScriptEngine::InitMono()
    {
        mono_set_assemblies_path(LV_DIR"/Limnova/thirdparty/mono/lib.NET");

        MonoDomain* rootDomain = mono_jit_init("LimnovaJITRuntime");
        LV_CORE_ASSERT(rootDomain, "Failed to initialize JIT!");
        s_SEData->RootDomain = rootDomain;

        static char appDomainName[] = "LimnovaScriptRuntime";
        s_SEData->AppDomain = mono_domain_create_appdomain(appDomainName, nullptr);
        mono_domain_set(s_SEData->AppDomain, true);

        s_SEData->CoreAssembly = LoadMonoAssembly(LV_SCRIPT_CORE_ASSEMBLY_PATH);
        s_SEData->CoreAssemblyImage = mono_assembly_get_image(s_SEData->CoreAssembly);
    }

    void ScriptEngine::ShutdownMono()
    {
        mono_domain_unload(s_SEData->AppDomain);
        s_SEData->AppDomain = nullptr;

        //mono_jit_cleanup(s_SEData->RootDomain); // TODO : make work ?
        s_SEData->RootDomain = nullptr;
    }

    MonoAssembly* ScriptEngine::LoadMonoAssembly(std::filesystem::path const& assemblyPath)
    {
        uint32_t fileSize = 0;
        char* fileData = Utils::ReadBytes(assemblyPath, &fileSize);

        // NOTE: We can't use this image for anything other than loading the assembly because this image doesn't have a reference to the assembly
        MonoImageOpenStatus status;
        MonoImage* image = mono_image_open_from_data_full(fileData, fileSize, 1, &status, 0);

        if (status != MONO_IMAGE_OK)
        {
            const char* errorMessage = mono_image_strerror(status);
            // Log some error message using the errorMessage data
            return nullptr;
        }

        MonoAssembly* assembly = mono_assembly_load_from_full(image, assemblyPath.string().c_str(), &status, 0);
        mono_image_close(image);

        // Don't forget to free the file data
        delete[] fileData;

        return assembly;
    }


    void ScriptEngine::PrintAssemblyTypes(MonoAssembly* assembly)
    {
        MonoImage* image = mono_assembly_get_image(assembly);
        const MonoTableInfo* typeDefinitionsTable = mono_image_get_table_info(image, MONO_TABLE_TYPEDEF);
        int32_t numTypes = mono_table_info_get_rows(typeDefinitionsTable);

        for (int32_t i = 0; i < numTypes; i++)
        {
            uint32_t cols[MONO_TYPEDEF_SIZE];
            mono_metadata_decode_row(typeDefinitionsTable, i, cols, MONO_TYPEDEF_SIZE);

            const char* nameSpace = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAMESPACE]);
            const char* name = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAME]);

            LV_CORE_INFO("{}.{}", nameSpace, name);
        }
    }

}

#undef SCRIPT_CORE_ASSEMBLY_PATH_BASE
#undef SCRIPT_CORE_ASSEMBLY_CONFIG
