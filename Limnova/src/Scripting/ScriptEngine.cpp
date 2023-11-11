#include "ScriptEngine.h"

#include "ScriptLibrary.h"
#include "Scene/Scene.h"

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>

#include <string>

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

        ScriptLibrary::RegisterComponentTypes(s_SEData->CoreAssemblyImage); // called after InitMono() has initialized CoreAssemblyImage
        ScriptLibrary::RegisterInternalCalls();

        // testing
        auto rPlayerClass = RegisterScriptClass<EntityScriptClass>("Player");

        //s_SEData->ScriptClass_Main = RegisterScriptClass<DynamicScriptClass>("Main");
        //size_t method_PrintVec3 = s_SEData->ScriptClass_Main->RegisterMethod("PrintVec3", 3);

        //Ref<DynamicScriptInstance> instance = s_SEData->ScriptClass_Main->Instantiate(s_SEData->AppDomain); // Main() constructor

        //float x = 0.1f, y = 2.3f, z = 4.5f;
        //float* vecParams[] = { &x, &y, &z };
        //s_SEData->ScriptClass_Main.InvokeMethod(method_PrintVec3, instance, (void**)vecParams);
        //instance->InvokeMethod(method_PrintVec3, (void**)vecParams);

        //RegisterScriptClass<EntityScriptClass>("Entity");
    }

    void ScriptEngine::Shutdown()
    {
        ShutdownMono();
        delete s_SEData;
        s_SEData = nullptr;
    }


    bool ScriptEngine::IsRegisteredScriptClass(std::string const& className)
    {
        return s_SEData->ScriptClasses.contains(className);
    }

    void ScriptEngine::OnSceneStart(Scene* scene)
    {
        s_SEData->SceneCtx = scene;

        scene->m_Registry.view<ScriptComponent>().each([=](auto entity, auto& sc)
        {
            IDComponent const& idc = scene->GetComponent<IDComponent>(entity);
            sc.HasScriptInstance = TryCreateEntityScript(idc.ID, sc.Name);
        });
    }

    void ScriptEngine::OnSceneUpdate(Scene* scene, Timestep dT)
    {
        void* pDT = (void*)&dT;
        for (size_t s = 0; s < s_SEData->EntityScriptInstances.size(); s++)
        {
            s_SEData->EntityScriptInstances[s]->InvokeOnUpdate((void**)&pDT);
        }
    }

    void ScriptEngine::OnSceneStop()
    {
        s_SEData->SceneCtx = nullptr;

        /// TODO : delete Mono objects ???

        s_SEData->EntityScriptInstances.clear();
        s_SEData->EntityScripts.clear();
    }

    bool ScriptEngine::TryCreateEntityScript(UUID entityId, std::string const& className)
    {
        auto itScriptClass = s_SEData->ScriptClasses.find(className);
        if (itScriptClass == s_SEData->ScriptClasses.end() ||
            itScriptClass->second->GetType() != ScriptClass::Type::Entity)
        {
            return false;
        }

        LV_CORE_ASSERT(!s_SEData->EntityScripts.contains(entityId), "Entity already has a script!");

        s_SEData->EntityScripts.emplace(entityId, s_SEData->EntityScriptInstances.size());
        auto& rNewInstance = s_SEData->EntityScriptInstances.emplace_back(
            CastRef<EntityScriptClass>(itScriptClass->second)->Instantiate(s_SEData->AppDomain)
        );

        void* arg[] = { &entityId };
        rNewInstance->InvokeOnCreate(arg);
        return true;
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


    /*** ScriptEngine::ScriptClass ***/

    ScriptEngine::ScriptClass::ScriptClass(std::string const& className)
    {
        LV_CORE_ASSERT(m_MonoClass == nullptr, "ScriptClass instance is already initialized!");
        m_MonoClass = mono_class_from_name(s_SEData->CoreAssemblyImage, "Limnova", className.c_str());
        m_ClassName = className;
    }

    Ref<ScriptEngine::ScriptInstance> ScriptEngine::ScriptClass::Instantiate(MonoDomain* domain)
    {
        LV_CORE_ASSERT(m_MonoClass != nullptr, "ScriptClass has not been initialized!");

        return CreateRef<ScriptInstance>(this, domain);
    }


    /*** ScriptEngine::DynamicScriptClass ***/

    std::hash<std::string> ScriptEngine::DynamicScriptClass::s_StringHasher = {};

    ScriptEngine::DynamicScriptClass::DynamicScriptClass(std::string const& className)
        : ScriptClass(className)
    {
        m_Type = Type::Dynamic;
    }

    Ref<ScriptEngine::DynamicScriptInstance> ScriptEngine::DynamicScriptClass::Instantiate(MonoDomain* domain)
    {
        LV_CORE_ASSERT(m_MonoClass != nullptr, "DynamicScriptClass has not been initialized!");

        return CreateRef<DynamicScriptInstance>(this, domain);
    }

    size_t ScriptEngine::DynamicScriptClass::RegisterMethod(std::string const& methodName, size_t numArgs)
    {
        LV_CORE_ASSERT(m_MonoClass != nullptr, "ScriptClass has not been initialized!");

        MonoMethod* monoMethod = mono_class_get_method_from_name(m_MonoClass, methodName.c_str(), numArgs);
        LV_CORE_ASSERT(monoMethod, "Failed to create MonoMethod!");

        size_t methodNameHash = s_StringHasher(methodName);
        m_Methods.emplace(methodNameHash, monoMethod);
        return methodNameHash;
    }

    MonoMethod* ScriptEngine::DynamicScriptClass::GetMethod(size_t methodNameHash)
    {
        return m_Methods.at(methodNameHash);
    }


    /*** ScriptEngine::EntityScriptClass ***/

    ScriptEngine::EntityScriptClass::EntityScriptClass(std::string const& className)
        : ScriptClass(className)
    {
        m_Type = Type::Entity;

        m_OnCreate = mono_class_get_method_from_name(m_MonoClass, "OnCreate", 1);
        m_OnUpdate = mono_class_get_method_from_name(m_MonoClass, "OnUpdate", 1);

        LV_CORE_ASSERT(m_OnCreate != nullptr && m_OnUpdate != nullptr,
            "Could not find required method implementation in given class!");
    }

    Ref<ScriptEngine::EntityScriptInstance> ScriptEngine::EntityScriptClass::Instantiate(MonoDomain* domain)
    {
        LV_CORE_ASSERT(m_MonoClass != nullptr, "ScriptClass has not been initialized!");

        return CreateRef<EntityScriptInstance>(this, domain);
    }


    /*** ScriptEngine::ScriptInstance ***/

    ScriptEngine::ScriptInstance::ScriptInstance(ScriptClass* scriptClass, MonoDomain* domain)
    {
        m_Instance = mono_object_new(domain, scriptClass->GetMonoClass());
    }


    /*** ScriptEngine::DynamicScriptInstance ***/

    ScriptEngine::DynamicScriptInstance::DynamicScriptInstance(DynamicScriptClass* dynamicScriptClass, MonoDomain* domain) :
        ScriptInstance((ScriptClass*)dynamicScriptClass, domain),
        m_scriptClass(dynamicScriptClass)
    {
    }

    void ScriptEngine::DynamicScriptInstance::InvokeMethod(size_t methodNameHash, void** arguments)
    {
        mono_runtime_invoke(m_scriptClass->GetMethod(methodNameHash), m_Instance, arguments, nullptr);
    }


    /*** ScriptEngine::EntityScriptInstance ***/

    ScriptEngine::EntityScriptInstance::EntityScriptInstance(EntityScriptClass* entityScriptClass, MonoDomain* domain) :
        ScriptInstance((ScriptClass*)entityScriptClass, domain),
        m_scriptClass(entityScriptClass)
    {
    }

    void ScriptEngine::EntityScriptInstance::InvokeOnCreate(void** argId)
    {
        mono_runtime_invoke(m_scriptClass->m_OnCreate, m_Instance, argId, nullptr);
    }

    void ScriptEngine::EntityScriptInstance::InvokeOnUpdate(void** argTimestep)
    {
        mono_runtime_invoke(m_scriptClass->m_OnUpdate, m_Instance, argTimestep, nullptr);
    }

}

#undef SCRIPT_CORE_ASSEMBLY_PATH_BASE
#undef SCRIPT_CORE_ASSEMBLY_CONFIG
