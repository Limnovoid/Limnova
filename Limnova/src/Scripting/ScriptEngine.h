#pragma once

#include "Core/UUID.h"
#include "Core/Timestep.h"

#include <filesystem>

extern "C" {
    typedef struct _MonoAssembly MonoAssembly;
    typedef struct _MonoDomain MonoDomain;
    typedef struct _MonoImage MonoImage;
    typedef struct _MonoClass MonoClass;
    typedef struct _MonoObject MonoObject;
    typedef struct _MonoMethod MonoMethod;
}


namespace Limnova
{
    class Scene;

    class ScriptEngine
    {
    public:
        ScriptEngine() = delete;

        static void Init();
        static void Shutdown();

        class ScriptInstance;
        class DynamicScriptInstance;
        class EntityScriptInstance;

        template<class T>
        static Ref<T> RegisterScriptClass(std::string const& className)
        {
            Ref<T> rClass = CreateRef<T>(className);
            s_SEData->ScriptClasses.emplace(className, CastRef<ScriptClass, T>(rClass));
            return rClass;
        }
        static bool IsRegisteredScriptClass(std::string const& className);

        static void OnSceneStart(Scene* scene);
        static void OnSceneUpdate(Scene* scene, Timestep dT);
        static void OnSceneStop();

        static Scene* GetContext() { return s_SEData->SceneCtx; }
    private:
        /// <summary>
        /// Creates a script class instance and associates it to the given entity ID.
        /// 'className' must be the valid name of a registered script class.
        /// </summary>
        /// <returns>true if a script class instance was created, otherwise false</returns>
        static bool TryCreateEntityScript(UUID entityId, std::string const& className);

        static void InitMono();
        static void ShutdownMono();

        static MonoAssembly* LoadMonoAssembly(std::filesystem::path const& assemblyPath);
        static void PrintAssemblyTypes(MonoAssembly* assembly);
    private:
        // -------------------------------------------------------------------------------------------------------------------------

        class ScriptClass
        {
        public:
            enum class Type
            {
                Dynamic,
                Entity,
                NUM
            };
        protected:
            std::string m_ClassName = "";
            MonoClass* m_MonoClass = nullptr;
            Type m_Type = Type::NUM;
        public:
            /// <summary> Constructs and initializes a ScriptClass. Calling Initialize() on a ScriptClass instance constructed with this constructor will fail an assert! </summary>
            ScriptClass(std::string const& className);

            /// <summary> Returns the stored pointer to the Mono object associated with this ScriptClass. </summary>
            MonoClass* GetMonoClass() const { return m_MonoClass; }

            Type GetType() const { return m_Type; }

            /// <summary> Creates (and returns the pointer to) a Mono object representing an instance of this ScriptClass. </summary>
            Ref<ScriptInstance> Instantiate(MonoDomain* domain);
        };

        // -------------------------------------------------------------------------------------------------------------------------

        class DynamicScriptClass : public ScriptClass
        {
            static std::hash<std::string> s_StringHasher;

            std::unordered_map<size_t, MonoMethod*> m_Methods = {};
        public:
            DynamicScriptClass(std::string const& className);

            /// <summary> Creates (and returns the pointer to) a Mono object representing an instance of this ScriptClass. </summary>
            Ref<DynamicScriptInstance> Instantiate(MonoDomain* domain);

            /// <summary> Registers a method in this class. </summary>
            /// <returns>The hash of the method name: the MonoMethod* associated with the registered method is stored in a map of string hashes (std::unordered_map size_t,MonoMethod* ).
            /// You may choose to store the string hash to avoid recomputing it where possible.</returns>
            size_t RegisterMethod(std::string const& methodName, size_t numArgs = 0);

            MonoMethod* GetMethod(size_t methodNameHash);
            MonoMethod* GetMethod(std::string const& methodName) { return GetMethod(s_StringHasher(methodName)); }

            /// <summary> Hashes the given string with the same std::hash object used internally to hash method names, and returns the hash. </summary>
            static size_t GetHashedName(std::string const& methodName) { return s_StringHasher(methodName); }
        };

        // -------------------------------------------------------------------------------------------------------------------------

        class EntityScriptClass : public ScriptClass
        {
            friend class EntityScriptInstance;

            MonoMethod* m_OnCreate = nullptr;
            MonoMethod* m_OnUpdate = nullptr;
        public:
            /// <summary> Constructs and initializes a ScriptClass. Calling Initialize() on a ScriptClass instance constructed with this constructor will fail an assert! </summary>
            EntityScriptClass(std::string const& className);

            /// <summary> Creates (and returns the pointer to) a Mono object representing an instance of this ScriptClass. </summary>
            Ref<EntityScriptInstance> Instantiate(MonoDomain* domain);
        };

        // -------------------------------------------------------------------------------------------------------------------------

        class ScriptInstance
        {
        public:
            ScriptInstance(ScriptClass* scriptClass, MonoDomain* domain);
        protected:
            MonoObject* m_Instance = nullptr;
        };

        // -------------------------------------------------------------------------------------------------------------------------

        class DynamicScriptInstance : public ScriptInstance
        {
        public:
            DynamicScriptInstance(DynamicScriptClass* dynamicScriptClass, MonoDomain* domain);

            void InvokeMethod(size_t methodNameHash, void** arguments = nullptr);
            void InvokeMethod(std::string const& methodName, void** arguments = nullptr) {
                InvokeMethod(((DynamicScriptClass*)m_scriptClass)->GetHashedName(methodName), arguments);
            }
        private:
            DynamicScriptClass* m_scriptClass = nullptr;
        };

        // -------------------------------------------------------------------------------------------------------------------------

        class EntityScriptInstance : public ScriptInstance
        {
        public:
            EntityScriptInstance(EntityScriptClass* entityScriptClass, MonoDomain* domain);

            void InvokeOnCreate(void** argId);
            void InvokeOnUpdate(void** argTimestep);
        private:
            EntityScriptClass* m_scriptClass;
        };

    private:
        // -------------------------------------------------------------------------------------------------------------------------

        struct ScriptEngineData
        {
            MonoDomain* RootDomain = nullptr;
            MonoDomain* AppDomain = nullptr;

            MonoAssembly* CoreAssembly = nullptr;
            MonoImage* CoreAssemblyImage = nullptr;

            std::unordered_map<std::string, Ref<ScriptClass>> ScriptClasses = {};

            std::unordered_map<UUID, size_t> EntityScripts = {};
            std::vector<Ref<EntityScriptInstance>> EntityScriptInstances = {};

            Scene* SceneCtx = nullptr;

            // testing
            Ref<DynamicScriptClass> ScriptClass_Main;
        };

        static ScriptEngineData* s_SEData;
    };

}
