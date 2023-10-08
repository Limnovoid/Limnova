#pragma once

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
    public:
        class ScriptClass
        {
            friend class ScriptLibrary;
            MonoClass* m_MonoClass = nullptr;
            std::map<size_t, MonoMethod*> m_Methods = {};
        public:
            /// <summary> Constructs an un-initialized ScriptClass. </summary>
            ScriptClass() = default;
            /// <summary> Constructs and initializes a ScriptClass. </summary>
            ScriptClass(std::string const& className);

            /// <summary> Initializes a ScriptClass object by creating (and storing a pointer to) its associated Mono object. </summary>
            void Initialize(std::string const& className);

            /// <summary> Returns the stored pointer to the Mono object associated with this ScriptClass. </summary>
            MonoClass* GetMonoClass() { return m_MonoClass; }

            /// <summary> Creates (and returns the pointer to) a Mono object representing an instance of this ScriptClass. </summary>
            MonoObject* Instantiate(MonoDomain* domain);

            /// <summary> Registers a method in this class. </summary>
            /// <returns>The hash of the method name: the MonoMethod* associated with the registered method is stored in a map of string hashes (std::map size_t,MonoMethod* ).
            /// You may choose to store the string hash to avoid recomputing it where possible.</returns>
            size_t RegisterMethod(std::string const& methodName, size_t numArgs = 0);

            MonoMethod* GetMethod(size_t methodNameHash);
            MonoMethod* GetMethod(std::string const& methodName) { return GetMethod(s_StringHasher(methodName)); }

            void InvokeMethod(size_t methodNameHash, MonoObject* instance, void** arguments = nullptr);
            void InvokeMethod(std::string const& methodName, MonoObject* instance, void** arguments = nullptr) {
                InvokeMethod(s_StringHasher(methodName), instance, arguments);
            }
        public:
            /// <summary> Hashes the given string with the same std::hash object used internally to hash method names, and returns the hash. </summary>
            static size_t GetHashedName(std::string const& methodName) { return s_StringHasher(methodName); }
        private:
            static std::hash<std::string> s_StringHasher;
        };
    private:
        struct ScriptEngineData
        {
            MonoDomain* RootDomain = nullptr;
            MonoDomain* AppDomain = nullptr;

            MonoAssembly* CoreAssembly = nullptr;
            MonoImage* CoreAssemblyImage = nullptr;

            ScriptClass ScriptClass_Main;
        };
        static ScriptEngineData* s_SEData;
    };

}
