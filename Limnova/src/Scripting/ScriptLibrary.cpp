#include "ScriptLibrary.h"

#include <mono/metadata/object.h>

#define LV_ADD_INTERNAL_CALL(func) mono_add_internal_call("Limnova.InternalCalls::" #func, func)

#define LV_SCRIPT_CLASS_REGISTER(assemblyImage, ClassName) \
    ScriptClass_##ClassName::s_MonoClass = mono_class_from_name(assemblyImage, "Limnova", #ClassName);

#define LV_SCRIPT_CLASS_REGISTER_METHOD(ClassName, MethodName, numArgs) {                                                   \
    MonoMethod* monoMethod = mono_class_get_method_from_name(ScriptClass_##ClassName::s_MonoClass, #MethodName, numArgs);   \
    ScriptClass_##ClassName::s_Methods.insert({ s_StringHasher(#MethodName), monoMethod }); }

#define LV_SCRIPT_CLASS_DEFINITIONS(ClassName)                                                                                          \
    MonoClass* ScriptLibrary::ScriptClass_##ClassName::s_MonoClass = nullptr;                                                           \
    std::map<size_t, MonoMethod*> ScriptLibrary::ScriptClass_##ClassName::s_Methods = {};                                               \
    MonoObject* ScriptLibrary::ScriptClass_##ClassName::Instantiate(MonoDomain* domain) {                                               \
        MonoObject* instance = mono_object_new(domain, s_MonoClass);                                                                    \
        mono_runtime_object_init(instance);                                                                                             \
        return instance; }                                                                                                              \
    MonoMethod* ScriptLibrary::ScriptClass_##ClassName::GetMethod(std::string const& methodName) {                                      \
        return s_Methods.at(s_StringHasher(methodName)); }                                                                              \
    void ScriptLibrary::ScriptClass_##ClassName::InvokeMethod(std::string const& methodName, MonoObject* instance, void** arguments) {  \
        mono_runtime_invoke(GetMethod(methodName), instance, arguments, nullptr); }


namespace Limnova
{

    static void NativeLog(MonoString* message)
    {
        char* cStr = mono_string_to_utf8(message);
        std::string str = { cStr };
        mono_free(cStr);
        LV_INFO(str);
    }


    std::hash<std::string> ScriptLibrary::s_StringHasher = {};


    void ScriptLibrary::RegisterAllFunctions()
    {
        LV_ADD_INTERNAL_CALL(NativeLog);
    }

    void ScriptLibrary::RegisterAllScriptClasses(MonoImage* coreAssemblyImage)
    {
        LV_SCRIPT_CLASS_REGISTER(coreAssemblyImage, Main);
        LV_SCRIPT_CLASS_REGISTER_METHOD(Main, PrintMessage, 0);
    }


    LV_SCRIPT_CLASS_DEFINITIONS(Main);
}
