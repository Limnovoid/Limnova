#pragma once

#include <filesystem>
#include <string>

#include "Core/UUID.h"
#include "Core/Timestep.h"

#include "Math/LimnovaMath.h"

extern "C" {
    typedef struct _MonoAssembly MonoAssembly;
    typedef struct _MonoDomain MonoDomain;
    typedef struct _MonoImage MonoImage;
    typedef struct _MonoClass MonoClass;
    typedef struct _MonoObject MonoObject;
    typedef struct _MonoMethod MonoMethod;
    typedef struct _MonoType MonoType;
    typedef struct _MonoClassField MonoClassField;
}


namespace Limnova
{

#define LV_SCRIPT_ENGINE_FIELD_LIST(d)                                                          \
    d(SCRIPT_FIELD_TYPE_FLOAT,        float,        Float,          "System.Single"             )\
    d(SCRIPT_FIELD_TYPE_DOUBLE,       double,       Double,         "System.Double"             )\
    d(SCRIPT_FIELD_TYPE_BOOL,         bool,         Bool,           "System.Boolean"            )\
    d(SCRIPT_FIELD_TYPE_SHORT,        int16_t,      Short,          "System.Int16"              )\
    d(SCRIPT_FIELD_TYPE_INT,          int32_t,      Int,            "System.Int32"              )\
    d(SCRIPT_FIELD_TYPE_LONG,         int64_t,      Long,           "System.Int64"              )\
    d(SCRIPT_FIELD_TYPE_USHORT,       uint16_t,     UShort,         "System.UInt16"             )\
    d(SCRIPT_FIELD_TYPE_UINT,         uint32_t,     UInt,           "System.UInt32"             )\
    d(SCRIPT_FIELD_TYPE_ULONG,        uint64_t,     ULong,          "System.UInt64"             )\
    d(SCRIPT_FIELD_TYPE_VECTOR2,      Vector2,      Vector2,        "Limnova.Vec2"              )\
    d(SCRIPT_FIELD_TYPE_VECTOR3,      Vector3,      Vector3,        "Limnova.Vec3"              )\
    d(SCRIPT_FIELD_TYPE_VECTOR3D,     Vector3d,     Vector3d,       "Limnova.Vec3d"             )\
    d(SCRIPT_FIELD_TYPE_ENTITY,       UUID,         EntityReference,"Limnova.EntityReference"   )
//    d(SCRIPT_FIELD_TYPE_BYTE,         uint8_t,      Byte,           "System.Byte"       )\
//    d(SCRIPT_FIELD_TYPE_CHAR,         char16_t,     Char,           "System.Char"       )

#define LV_SCRIPT_ENGINE_DECLARE_SCRIPT_FIELD_TYPES(id, type, name, monoTypeName) id,

#define LV_SCRIPT_ENGINE_MAP_MONO_TYPE_NAME_TO_SCRIPT_FIELD_TYPE(id, type, name, monoTypeName)  s_pData->ScriptFieldTypes.emplace(monoTypeName, ScriptFieldType::id);

#define LV_SCRIPT_ENGINE_FIELD_TYPE_TO_STRING(id, type, name, monoTypeName) case ScriptFieldType::id: return #name;

#define LV_SCRIPT_ENGINE_FIELD_UNION_TYPE_DECLARATION(id, type, name, monoTypeName) type m_##name;

#define LV_SCRIPT_ENGINE_FIELD_INSTANCE_IS_CORRECT_TYPE_SPECIALIZATION(id, type, name, monoTypeName)\
    template<> bool IsCorrectType<type>() const { return m_FieldClass.GetType() == id; }

// ---------------------------------------------------------------------------------------------------------------------------------

    class Scene;

    class ScriptEngine
    {
    public:
        struct Context;

        static constexpr size_t sizeChar = sizeof(char16_t);

        enum ScriptFieldType
        {
            LV_SCRIPT_ENGINE_FIELD_LIST(LV_SCRIPT_ENGINE_DECLARE_SCRIPT_FIELD_TYPES)
            SCRIPT_FIELD_TYPE_NUM
        };

        ScriptEngine() = delete;

        static void Initialize();
        static void Shutdown();

        class ScriptInstance;
        class DynamicScriptInstance;
        class EntityScriptInstance;

        template<class T>
        static Ref<T> RegisterScriptClass(std::string const& className)
        {
            Ref<T> rClass = CreateRef<T>(className);
            s_pData->ScriptClasses.emplace(className, CastRef<ScriptClass, T>(rClass));
            return rClass;
        }
        static bool IsRegisteredScriptClass(std::string const& className);

        static void SetContext(Scene *pScene);
        static Scene *GetContext();

        static const Ref<EntityScriptInstance> &GetEntityScriptInstance(UUID entityId);

        static void OnSceneStart(Scene* pScene);
        static void OnSceneUpdate(Timestep dT);
        static void OnSceneStop();

        /// <summary>
        /// Creates a script class instance and associates it with the given entity ID.
        /// 'className' must be the valid name of a registered script class.
        /// Replaces the existing script if the entity ID is already associated with one.
        /// </summary>
        /// <returns>true if a script class instance was created, otherwise false</returns>
        static bool TryCreateEntityScript(UUID entityId, std::string const& className);

        static bool TryDeleteEntityScript(UUID entityId);

        static const char *FieldTypeToString(ScriptFieldType fieldType);
    private:
        static size_t GetFreeScriptInstanceIndex();

        static void CompressScriptInstanceVector();
        static size_t FindLastNotFreeScriptInstanceIndex(size_t initialIndex);

        static void InitMono();
        static void ShutdownMono();

        static MonoAssembly* LoadMonoAssembly(std::filesystem::path const& assemblyPath);
        static void PrintAssemblyTypes(MonoAssembly* assembly);

        static ScriptFieldType GetScriptFieldType(MonoType *pMonoType);

    public:
        // -------------------------------------------------------------------------------------------------------------------------

        class FieldClass
        {
            ScriptFieldType m_Type;
            MonoClassField* m_MonoField;

        public:
            FieldClass(ScriptFieldType fieldType, MonoClassField *monoField)
                : m_Type(fieldType), m_MonoField(monoField)
            {
            }

            ScriptFieldType GetType() const { return m_Type; }
            MonoClassField* GetMonoField() const { return m_MonoField; }
        };

        // -------------------------------------------------------------------------------------------------------------------------

        typedef std::unordered_map<std::string, Ref<FieldClass>> FieldClassMap;

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

            FieldClassMap m_Fields;
        public:
            /// <summary> Constructs and initializes a ScriptClass. Calling Initialize() on a ScriptClass instance constructed with this constructor will fail an assert! </summary>
            ScriptClass(std::string const& className);

            /// <summary> Returns the stored pointer to the Mono object associated with this ScriptClass. </summary>
            MonoClass* GetMonoClass() const
            {
                return m_MonoClass;
            }

            Type GetType() const
            {
                return m_Type;
            }

            /// <summary> Creates (and returns the pointer to) a Mono object representing an instance of this ScriptClass. </summary>
            Ref<ScriptInstance> Instantiate(MonoDomain* domain);

            const FieldClassMap &GetFields() const
            {
                return m_Fields;
            }
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

        class FieldInstance
        {
            const FieldClass &m_FieldClass;
            const ScriptInstance &m_ScriptInstance;

        public:
            FieldInstance(const FieldClass &fieldClass, ScriptInstance &scriptInstance) :
                m_FieldClass(fieldClass), m_ScriptInstance(scriptInstance)
            {
            }

            ScriptFieldType GetType() const
            {
                return m_FieldClass.GetType();
            }

            template <typename T>
            void GetValue(T &value) const
            {
                LV_CORE_ASSERT(IsCorrectType<T>(), "Type parameter does not match field type!");
                GetValueInternal(static_cast<void *>(&value));
            }

            template <typename T>
            void SetValue(T &value) const
            {
                LV_CORE_ASSERT(IsCorrectType<T>(), "Type parameter does not match field type!");
                SetValueInternal(static_cast<void *>(&value));
            }

        private:
            void GetValueInternal(void *pValue) const;
            void SetValueInternal(void *pValue) const;

            template<typename T>
            bool IsCorrectType() const
            {
                return false;
            }

            LV_SCRIPT_ENGINE_FIELD_LIST(LV_SCRIPT_ENGINE_FIELD_INSTANCE_IS_CORRECT_TYPE_SPECIALIZATION)
        };

        // -------------------------------------------------------------------------------------------------------------------------

        typedef std::unordered_map<std::string, Ref<FieldInstance>> FieldInstanceMap;

        class ScriptInstance
        {
        public:
            ScriptInstance(ScriptClass* scriptClass, MonoDomain* domain);

            MonoObject *GetMonoObject() const
            {
                return m_Instance;
            }

            const FieldInstanceMap &GetFields() const
            {
                return m_Fields;
            }

        protected:
            void InitializeFields();

            virtual const ScriptClass *GetScriptClass() const
            {
                return nullptr;
            }

            MonoObject* m_Instance = nullptr;

        private:
            FieldInstanceMap m_Fields;
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

            virtual const ScriptClass *GetScriptClass() const override
            {
                return m_scriptClass;
            }
        };

        // -------------------------------------------------------------------------------------------------------------------------

        class EntityScriptInstance : public ScriptInstance
        {
            EntityScriptClass* m_scriptClass;
        public:
            EntityScriptInstance(EntityScriptClass* entityScriptClass, MonoDomain* domain);

            void InvokeOnCreate(void** argId) const;
            void InvokeOnUpdate(void** argTimestep) const;

        private:
            virtual const ScriptClass *GetScriptClass() const override
            {
                return m_scriptClass;
            }
        };

        // -------------------------------------------------------------------------------------------------------------------------

        struct StaticData
        {
            MonoDomain* RootDomain = nullptr;
            MonoDomain* AppDomain = nullptr;

            MonoAssembly* CoreAssembly = nullptr;
            MonoImage* CoreAssemblyImage = nullptr;

            std::unordered_map<std::string, Ref<ScriptClass>> ScriptClasses = {};
            std::unordered_map<std::string, ScriptFieldType> ScriptFieldTypes;
        };

        struct Context
        {
            std::unordered_map<UUID, size_t> EntityScriptIndices = {};
            std::unordered_set<size_t> FreeScriptIndices = {};
            std::vector<Ref<EntityScriptInstance>> EntityScriptInstances = {};

            // testing
            Ref<DynamicScriptClass> ScriptClass_Main;
        };

        static StaticData *s_pData;
        static Context *s_pContext;
        static Scene *s_pScene;
    };

}
