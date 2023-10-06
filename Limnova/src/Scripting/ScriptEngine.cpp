#include "ScriptEngine.h"

#include "mono/jit/jit.h"
#include "mono/metadata/assembly.h"

#define SCRIPT_CORE_ASSEMBLY_PATH_BASE LV_DIR"/LimnovaEditor/Resources/lib/Scripting/"
#ifdef LV_DEBUG
    #define SCRIPT_CORE_ASSEMBLY_CONFIG "Debug"
#elif LV_RELEASE
    #define SCRIPT_CORE_ASSEMBLY_CONFIG "Release"
#endif
#define LV_SCRIPT_CORE_ASSEMBLY_PATH SCRIPT_CORE_ASSEMBLY_PATH_BASE SCRIPT_CORE_ASSEMBLY_CONFIG "/LimnovaScriptCore.dll"

namespace Limnova
{

    struct ScriptEngineData
    {
        MonoDomain* RootDomain;
        MonoDomain* AppDomain;

        MonoAssembly* CoreAssembly;
    };

    static ScriptEngineData* s_SEData;

    void ScriptEngine::Init()
    {
        s_SEData = new ScriptEngineData;
        InitMono();
    }

    void ScriptEngine::Shutdown()
    {
        ShutdownMono();
        delete s_SEData;
    }


    char* ReadBytes(const std::string& filepath, uint32_t* outSize)
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

    MonoAssembly* LoadCSharpAssembly(const std::string& assemblyPath)
    {
        uint32_t fileSize = 0;
        char* fileData = ReadBytes(assemblyPath, &fileSize);

        // NOTE: We can't use this image for anything other than loading the assembly because this image doesn't have a reference to the assembly
        MonoImageOpenStatus status;
        MonoImage* image = mono_image_open_from_data_full(fileData, fileSize, 1, &status, 0);

        if (status != MONO_IMAGE_OK)
        {
            const char* errorMessage = mono_image_strerror(status);
            // Log some error message using the errorMessage data
            return nullptr;
        }

        MonoAssembly* assembly = mono_assembly_load_from_full(image, assemblyPath.c_str(), &status, 0);
        mono_image_close(image);

        // Don't forget to free the file data
        delete[] fileData;

        return assembly;
    }

    void PrintAssemblyTypes(MonoAssembly* assembly)
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


    void ScriptEngine::InitMono()
    {
        mono_set_assemblies_path(LV_DIR"/Limnova/thirdparty/mono/lib.NET");

        MonoDomain* rootDomain = mono_jit_init("LimnovaJITRuntime");
        LV_CORE_ASSERT(rootDomain, "Failed to initialize JIT!");

        s_SEData->RootDomain = rootDomain;

        static char appDomainName[32];
        strcpy(appDomainName, "LimnovaScriptRuntime");
        s_SEData->AppDomain = mono_domain_create_appdomain(appDomainName, nullptr);
        mono_domain_set(s_SEData->AppDomain, true);

        s_SEData->CoreAssembly = LoadCSharpAssembly(LV_SCRIPT_CORE_ASSEMBLY_PATH);
        PrintAssemblyTypes(s_SEData->CoreAssembly);


        // Test Main
        MonoImage* assemblyImage = mono_assembly_get_image(s_SEData->CoreAssembly);
        MonoClass* monoClass = mono_class_from_name(assemblyImage, "Limnova", "Main");

        // 1. create object and call constructor
        MonoObject* instance = mono_object_new(s_SEData->AppDomain, monoClass);
        mono_runtime_object_init(instance);

        // 2. call function
        MonoMethod* printMessageFunc = mono_class_get_method_from_name(monoClass, "PrintMessage", 0);
        mono_runtime_invoke(printMessageFunc, instance, nullptr, nullptr);
    }

    void ScriptEngine::ShutdownMono()
    {
        mono_domain_unload(s_SEData->AppDomain);
        s_SEData->AppDomain = nullptr;

        //mono_jit_cleanup(s_SEData->RootDomain); // TODO : make work ?
        s_SEData->RootDomain = nullptr;
    }

}

#undef SCRIPT_CORE_ASSEMBLY_PATH_BASE
#undef SCRIPT_CORE_ASSEMBLY_CONFIG
