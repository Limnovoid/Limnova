#include "ScriptLibrary.h"

#include <mono/metadata/object.h>

#define LV_ADD_INTERNAL_CALL(func) mono_add_internal_call("Limnova.InternalCalls::" #func, func)


namespace Limnova
{

    static void NativeLog(MonoString* message)
    {
        char* cStr = mono_string_to_utf8(message);
        std::string str = { cStr };
        mono_free(cStr);
        LV_INFO(str);
    }




    void ScriptLibrary::RegisterAllFunctions()
    {
        LV_ADD_INTERNAL_CALL(NativeLog);
    }

}
