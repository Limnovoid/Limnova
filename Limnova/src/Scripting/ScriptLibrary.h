#pragma once

#define LV_SCRIPT_CLASS(ClassName)                                  \
class ScriptClass_##ClassName {                                     \
    friend class ScriptLibrary;                                     \
    static MonoClass* s_MonoClass;                                  \
    static std::map<size_t, MonoMethod*> s_Methods;                 \
public:                                                             \
    ScriptClass_##ClassName() = delete;                             \
    static MonoClass* GetMonoClass() { return s_MonoClass; }        \
    static MonoObject* Instantiate(MonoDomain* domain);             \
    static MonoMethod* GetMethod(std::string const& methodName);    \
    static void InvokeMethod(std::string const& methodName, MonoObject* instance, void** arguments = nullptr); }

extern "C" {
    typedef struct _MonoDomain MonoDomain;
    typedef struct _MonoClass MonoClass;
    typedef struct _MonoObject MonoObject;
    typedef struct _MonoImage MonoImage;
    typedef struct _MonoMethod MonoMethod;
}


namespace Limnova
{

    class ScriptLibrary
    {
    public:
        static void RegisterAllFunctions();
        static void RegisterAllScriptClasses(MonoImage* coreAssemblyImage);

        LV_SCRIPT_CLASS(Main);
    private:
        static std::hash<std::string> s_StringHasher;
    };

}
