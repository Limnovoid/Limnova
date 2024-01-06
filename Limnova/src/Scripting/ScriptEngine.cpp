#include "ScriptEngine.h"

#include "ScriptLibrary.h"

#include "Scene/Scene.h"

#include <mono/jit/jit.h>

#include <mono/metadata/assembly.h>
#include <mono/metadata/attrdefs.h>
#include <mono/metadata/object.h>

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

    // -----------------------------------------------------------------------------------------------------------------------------
    // -----------------------------------------------------------------------------------------------------------------------------

    ScriptEngine::StaticData* ScriptEngine::s_pData = nullptr;
    ScriptEngine::Context* ScriptEngine::s_pContext = nullptr;
    Scene* ScriptEngine::s_pScene = nullptr;

    // -----------------------------------------------------------------------------------------------------------------------------

    void ScriptEngine::Initialize()
    {
        LV_CORE_ASSERT(s_pData == nullptr, "ScriptEngine is already initialized!");
        s_pData = new StaticData;
        InitMono();

        ScriptLibrary::RegisterComponentTypes(s_pData->CoreAssemblyImage); // called after InitMono() has initialized CoreAssemblyImage
        ScriptLibrary::RegisterInternalCalls();

        LV_SCRIPT_ENGINE_FIELD_LIST(LV_SCRIPT_ENGINE_MAP_MONO_TYPE_NAME_TO_SCRIPT_FIELD_TYPE);

        // testing
        auto rPlayerClass = RegisterScriptClass<EntityScriptClass>("Player");
    }

    // -----------------------------------------------------------------------------------------------------------------------------

    void ScriptEngine::Shutdown()
    {
        ShutdownMono();
        s_pScene = nullptr;
        s_pContext = nullptr;
    }

    // -----------------------------------------------------------------------------------------------------------------------------

    bool ScriptEngine::IsRegisteredScriptClass(std::string const& className)
    {
        return s_pData->ScriptClasses.contains(className);
    }

    // -----------------------------------------------------------------------------------------------------------------------------

    void ScriptEngine::SetContext(Scene *pScene)
    {
        s_pScene = pScene;
        s_pContext = &pScene->m_ScriptContext;
    }

    // -----------------------------------------------------------------------------------------------------------------------------

    Scene *ScriptEngine::GetContext()
    {
        return s_pScene;
    }

    // -----------------------------------------------------------------------------------------------------------------------------

    const Ref<ScriptEngine::EntityScriptInstance> &ScriptEngine::GetEntityScriptInstance(UUID entityId)
    {
        auto itEntityScriptIndex = s_pContext->EntityScriptIndices.find(entityId);
        if (itEntityScriptIndex == s_pContext->EntityScriptIndices.end())
            return nullptr;

        return s_pContext->EntityScriptInstances[itEntityScriptIndex->second];
    }

    // -----------------------------------------------------------------------------------------------------------------------------

    void ScriptEngine::OnSceneStart(Scene* pScene)
    {
        SetContext(pScene);
        CompressScriptInstanceVector();
    }

    // -----------------------------------------------------------------------------------------------------------------------------

    void ScriptEngine::OnSceneUpdate(Timestep dT)
    {
        void* pDT = (void*)&dT;
        for (size_t s = 0; s < s_pContext->EntityScriptInstances.size(); s++)
        {
            s_pContext->EntityScriptInstances[s]->InvokeOnUpdate((void**)&pDT);
        }
    }

    // -----------------------------------------------------------------------------------------------------------------------------

    void ScriptEngine::OnSceneStop()
    {
    }

    // -----------------------------------------------------------------------------------------------------------------------------

    bool ScriptEngine::TryCreateEntityScript(UUID entityId, std::string const& className)
    {
        auto itScriptClass = s_pData->ScriptClasses.find(className);
        if (itScriptClass == s_pData->ScriptClasses.end() ||
            itScriptClass->second->GetType() != ScriptClass::Type::Entity)
        {
            return false;
        }

        size_t scriptIndex;

        if (auto itScriptIndex = s_pContext->EntityScriptIndices.find(entityId);
            itScriptIndex != s_pContext->EntityScriptIndices.end())
        {
            scriptIndex = itScriptIndex->second;
        }
        else
        {
            scriptIndex = GetFreeScriptInstanceIndex();
            s_pContext->EntityScriptIndices.emplace(entityId, scriptIndex);
        }

        s_pContext->EntityScriptInstances[scriptIndex] = CastRef<EntityScriptClass>(itScriptClass->second)->Instantiate(s_pData->AppDomain);

        void* arg[] = { &entityId };
        s_pContext->EntityScriptInstances[scriptIndex]->InvokeOnCreate(arg);
        return true;
    }

    // -----------------------------------------------------------------------------------------------------------------------------

    bool ScriptEngine::TryDeleteEntityScript(UUID entityId)
    {
        auto itScriptIndex = s_pContext->EntityScriptIndices.find(entityId);
        if (itScriptIndex == s_pContext->EntityScriptIndices.end())
            return false;

        s_pContext->EntityScriptInstances[itScriptIndex->second] = nullptr;
        s_pContext->FreeScriptIndices.insert(itScriptIndex->second);
        s_pContext->EntityScriptIndices.erase(itScriptIndex);
        return true;
    }

    // -----------------------------------------------------------------------------------------------------------------------------

    const char *ScriptEngine::FieldTypeToString(ScriptFieldType fieldType)
    {
        switch (fieldType)
        {
            LV_SCRIPT_ENGINE_FIELD_LIST(LV_SCRIPT_ENGINE_FIELD_TYPE_TO_STRING)
        }
        LV_CORE_ERROR("Unknown field type!");
        return "Unknown";
    }

    // -----------------------------------------------------------------------------------------------------------------------------

    size_t ScriptEngine::GetFreeScriptInstanceIndex()
    {
        size_t freeIndex;
        if (s_pContext->FreeScriptIndices.size() != 0)
        {
            auto itFreeIndex = s_pContext->FreeScriptIndices.begin();
            freeIndex = *itFreeIndex;
            s_pContext->FreeScriptIndices.erase(itFreeIndex);
        }
        else
        {
            freeIndex = s_pContext->EntityScriptInstances.size();
            s_pContext->EntityScriptInstances.emplace_back(nullptr);
        }
        return freeIndex;
    }

    // -----------------------------------------------------------------------------------------------------------------------------

    void ScriptEngine::CompressScriptInstanceVector()
    {
        if (s_pContext->EntityScriptInstances.size() == 0)
            return;

        size_t lastNotFreeIndex = FindLastNotFreeScriptInstanceIndex(s_pContext->EntityScriptInstances.size() - 1);

        for (auto itFreeIndex = s_pContext->FreeScriptIndices.begin(); itFreeIndex != s_pContext->FreeScriptIndices.end(); ++itFreeIndex)
        {
            if (lastNotFreeIndex == 0 || lastNotFreeIndex < *itFreeIndex)
                break;

            LV_CORE_ASSERT(s_pContext->EntityScriptInstances[*itFreeIndex] == nullptr &&
                s_pContext->EntityScriptInstances[lastNotFreeIndex] != nullptr,
                "Invalid swap!");

            s_pContext->EntityScriptInstances[*itFreeIndex].swap(s_pContext->EntityScriptInstances[lastNotFreeIndex]);

            // Update entity mapping to script index
            auto itScriptIndices = s_pContext->EntityScriptIndices.begin();
            while (itScriptIndices->second != lastNotFreeIndex)
                ++itScriptIndices;
            UUID entityUuid = itScriptIndices->first;
            itScriptIndices->second = *itFreeIndex;

            LV_CORE_ASSERT(s_pContext->EntityScriptInstances[*itFreeIndex] != nullptr &&
                s_pContext->EntityScriptInstances[lastNotFreeIndex] == nullptr,
                "Swap failed!");

            lastNotFreeIndex = FindLastNotFreeScriptInstanceIndex(lastNotFreeIndex - 1);
        }

        s_pContext->EntityScriptInstances.resize(s_pContext->EntityScriptInstances.size() - s_pContext->FreeScriptIndices.size());
        s_pContext->FreeScriptIndices.clear();
    }

    // -----------------------------------------------------------------------------------------------------------------------------

    size_t ScriptEngine::FindLastNotFreeScriptInstanceIndex(size_t initialIndex)
    {
        size_t i = (initialIndex < s_pContext->EntityScriptInstances.size()) ?
            initialIndex : s_pContext->EntityScriptInstances.size() - 1;

        while (i > 0 && s_pContext->EntityScriptInstances[i] == nullptr)
            --i;

        return i;
    }

    // -----------------------------------------------------------------------------------------------------------------------------

    void ScriptEngine::InitMono()
    {
        mono_set_assemblies_path(LV_DIR"/Limnova/thirdparty/mono/lib.NET");

        MonoDomain* rootDomain = mono_jit_init("LimnovaJITRuntime");
        LV_CORE_ASSERT(rootDomain, "Failed to initialize JIT!");
        s_pData->RootDomain = rootDomain;

        static char appDomainName[] = "LimnovaScriptRuntime";
        s_pData->AppDomain = mono_domain_create_appdomain(appDomainName, nullptr);
        mono_domain_set(s_pData->AppDomain, true);

        s_pData->CoreAssembly = LoadMonoAssembly(LV_SCRIPT_CORE_ASSEMBLY_PATH);
        s_pData->CoreAssemblyImage = mono_assembly_get_image(s_pData->CoreAssembly);
    }

    // -----------------------------------------------------------------------------------------------------------------------------

    void ScriptEngine::ShutdownMono()
    {
        mono_domain_unload(s_pData->AppDomain);
        s_pData->AppDomain = nullptr;

        //mono_jit_cleanup(s_pContext->RootDomain); // TODO : make work ?
        s_pData->RootDomain = nullptr;
    }

    // -----------------------------------------------------------------------------------------------------------------------------

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

    // -----------------------------------------------------------------------------------------------------------------------------

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

    // -----------------------------------------------------------------------------------------------------------------------------

    ScriptEngine::ScriptFieldType ScriptEngine::GetScriptFieldType(MonoType *pMonoType)
    {
        std::string typeName = mono_type_get_name(pMonoType);

        auto fieldTypeIterator = s_pData->ScriptFieldTypes.find(typeName);
        if (fieldTypeIterator == s_pData->ScriptFieldTypes.end())
            return ScriptFieldType::SCRIPT_FIELD_TYPE_NUM;

        return fieldTypeIterator->second;
    }

    // -----------------------------------------------------------------------------------------------------------------------------
    // -----------------------------------------------------------------------------------------------------------------------------

    ScriptEngine::ScriptClass::ScriptClass(std::string const& className)
    {
        LV_CORE_ASSERT(m_MonoClass == nullptr, "ScriptClass instance is already initialized!");
        m_MonoClass = mono_class_from_name(s_pData->CoreAssemblyImage, "Limnova", className.c_str());
        m_ClassName = className;

        // Get fields
        int nFields = mono_class_num_fields(m_MonoClass);
        LV_CORE_INFO("Class {0} has {1} fields", m_ClassName, nFields);

        void *pMonoFieldIterator = nullptr;
        MonoClassField *pMonoField = nullptr;
        while (nullptr != (pMonoField = mono_class_get_fields(m_MonoClass, &pMonoFieldIterator)))
        {
            uint32_t flags = mono_field_get_flags(pMonoField);
            if (flags & MONO_FIELD_ATTR_PUBLIC)
            {
                const char *monoFieldName = mono_field_get_name(pMonoField);

                MonoType *pMonoFieldType = mono_field_get_type(pMonoField);
                ScriptFieldType fieldType = GetScriptFieldType(pMonoFieldType);

                m_Fields.emplace(monoFieldName, std::move(CreateRef<FieldClass>(fieldType, pMonoField)));

                LV_CORE_INFO(" - {} ({})", monoFieldName, FieldTypeToString(fieldType));
            }
        }
    }

    // -----------------------------------------------------------------------------------------------------------------------------

    Ref<ScriptEngine::ScriptInstance> ScriptEngine::ScriptClass::Instantiate(MonoDomain* domain)
    {
        LV_CORE_ASSERT(m_MonoClass != nullptr, "ScriptClass has not been initialized!");

        return CreateRef<ScriptInstance>(this, domain);
    }

    // -----------------------------------------------------------------------------------------------------------------------------
    // -----------------------------------------------------------------------------------------------------------------------------

    std::hash<std::string> ScriptEngine::DynamicScriptClass::s_StringHasher = {};

    ScriptEngine::DynamicScriptClass::DynamicScriptClass(std::string const& className)
        : ScriptClass(className)
    {
        m_Type = Type::Dynamic;
    }

    // -----------------------------------------------------------------------------------------------------------------------------

    Ref<ScriptEngine::DynamicScriptInstance> ScriptEngine::DynamicScriptClass::Instantiate(MonoDomain* domain)
    {
        LV_CORE_ASSERT(m_MonoClass != nullptr, "DynamicScriptClass has not been initialized!");

        return CreateRef<DynamicScriptInstance>(this, domain);
    }

    // -----------------------------------------------------------------------------------------------------------------------------

    size_t ScriptEngine::DynamicScriptClass::RegisterMethod(std::string const& methodName, size_t numArgs)
    {
        LV_CORE_ASSERT(m_MonoClass != nullptr, "ScriptClass has not been initialized!");

        MonoMethod* monoMethod = mono_class_get_method_from_name(m_MonoClass, methodName.c_str(), numArgs);
        LV_CORE_ASSERT(monoMethod, "Failed to create MonoMethod!");

        size_t methodNameHash = s_StringHasher(methodName);
        m_Methods.emplace(methodNameHash, monoMethod);
        return methodNameHash;
    }

    // -----------------------------------------------------------------------------------------------------------------------------

    MonoMethod* ScriptEngine::DynamicScriptClass::GetMethod(size_t methodNameHash)
    {
        return m_Methods.at(methodNameHash);
    }

    // -----------------------------------------------------------------------------------------------------------------------------
    // -----------------------------------------------------------------------------------------------------------------------------

    ScriptEngine::EntityScriptClass::EntityScriptClass(std::string const& className)
        : ScriptClass(className)
    {
        m_Type = Type::Entity;

        m_OnCreate = mono_class_get_method_from_name(m_MonoClass, "OnCreate", 1);
        m_OnUpdate = mono_class_get_method_from_name(m_MonoClass, "OnUpdate", 1);

        LV_CORE_ASSERT(m_OnCreate != nullptr && m_OnUpdate != nullptr,
            "Could not find required method implementation in given class!");
    }

    // -----------------------------------------------------------------------------------------------------------------------------

    Ref<ScriptEngine::EntityScriptInstance> ScriptEngine::EntityScriptClass::Instantiate(MonoDomain* domain)
    {
        LV_CORE_ASSERT(m_MonoClass != nullptr, "ScriptClass has not been initialized!");

        return CreateRef<EntityScriptInstance>(this, domain);
    }

    // -----------------------------------------------------------------------------------------------------------------------------
    // -----------------------------------------------------------------------------------------------------------------------------

    void ScriptEngine::FieldInstance::GetValueInternal(void *pValue) const
    {
        mono_field_get_value(m_ScriptInstance.GetMonoObject(), m_FieldClass.GetMonoField(), pValue);
    }

    // -----------------------------------------------------------------------------------------------------------------------------

    void ScriptEngine::FieldInstance::SetValueInternal(void *pValue) const
    {
        mono_field_set_value(m_ScriptInstance.GetMonoObject(), m_FieldClass.GetMonoField(), pValue);
    }

    // -----------------------------------------------------------------------------------------------------------------------------
    // -----------------------------------------------------------------------------------------------------------------------------

    ScriptEngine::ScriptInstance::ScriptInstance(ScriptClass* scriptClass, MonoDomain* domain) :
        m_Instance(mono_object_new(domain, scriptClass->GetMonoClass()))
    {
    }

    // -----------------------------------------------------------------------------------------------------------------------------

    void ScriptEngine::ScriptInstance::InitializeFields()
    {
        auto &fieldClasses = GetScriptClass()->GetFields();
        for (auto &fieldClass : fieldClasses)
            m_Fields.emplace(fieldClass.first, std::move(CreateRef<FieldInstance>(*(fieldClass.second.get()), *this)));
    }

    // -----------------------------------------------------------------------------------------------------------------------------
    // -----------------------------------------------------------------------------------------------------------------------------

    ScriptEngine::DynamicScriptInstance::DynamicScriptInstance(DynamicScriptClass* dynamicScriptClass, MonoDomain* domain) :
        ScriptInstance((ScriptClass*)dynamicScriptClass, domain),
        m_scriptClass(dynamicScriptClass)
    {
        ScriptInstance::InitializeFields();
    }

    // -----------------------------------------------------------------------------------------------------------------------------

    void ScriptEngine::DynamicScriptInstance::InvokeMethod(size_t methodNameHash, void** arguments)
    {
        mono_runtime_invoke(m_scriptClass->GetMethod(methodNameHash), m_Instance, arguments, nullptr);
    }

    // -----------------------------------------------------------------------------------------------------------------------------
    // -----------------------------------------------------------------------------------------------------------------------------

    ScriptEngine::EntityScriptInstance::EntityScriptInstance(EntityScriptClass* entityScriptClass, MonoDomain* domain) :
        ScriptInstance((ScriptClass*)entityScriptClass, domain),
        m_scriptClass(entityScriptClass)
    {
        ScriptInstance::InitializeFields();
    }

    // -----------------------------------------------------------------------------------------------------------------------------

    void ScriptEngine::EntityScriptInstance::InvokeOnCreate(void** argId) const
    {
        mono_runtime_invoke(m_scriptClass->m_OnCreate, m_Instance, argId, nullptr);
    }

    // -----------------------------------------------------------------------------------------------------------------------------

    void ScriptEngine::EntityScriptInstance::InvokeOnUpdate(void** argTimestep) const
    {
        mono_runtime_invoke(m_scriptClass->m_OnUpdate, m_Instance, argTimestep, nullptr);
    }

}

#undef SCRIPT_CORE_ASSEMBLY_PATH_BASE
#undef SCRIPT_CORE_ASSEMBLY_CONFIG
