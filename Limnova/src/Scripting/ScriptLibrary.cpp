#include "ScriptLibrary.h"

#include <mono/metadata/object.h>

#include <Math/Math.h>

#define LV_ADD_INTERNAL_CALL(func) mono_add_internal_call("Limnova.Native::" #func, InternalCall::func)


namespace Limnova
{

    static std::string ToString(MonoString* monoStr)
    {
        char* cStr = mono_string_to_utf8(monoStr);
        std::string str = { cStr };
        mono_free(cStr);
        return str;
    }

    namespace InternalCall
    {
        static void LogInfo(MonoString* message) { LV_INFO(ToString(message)); }
        static void LogTrace(MonoString* message) { LV_TRACE(ToString(message)); }
        static void LogWarn(MonoString* message) { LV_WARN(ToString(message)); }
        static void LogError(MonoString* message) { LV_ERROR(ToString(message)); }

        static void CrossVec3(Vector3* lhs, Vector3* rhs, Vector3* res)
        {
            *res = lhs->Cross(*rhs);
        }
    }


    /*** Register internal call functions ***/

    void ScriptLibrary::RegisterAllFunctions()
    {
        LV_ADD_INTERNAL_CALL(LogInfo);
        LV_ADD_INTERNAL_CALL(LogTrace);
        LV_ADD_INTERNAL_CALL(LogWarn);
        LV_ADD_INTERNAL_CALL(LogError);
        LV_ADD_INTERNAL_CALL(CrossVec3);
    }

}
