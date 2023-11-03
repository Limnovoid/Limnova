#include "ScriptLibrary.h"

#include "ScriptEngine.h"

#include <mono/metadata/object.h>

#include <Math/Math.h>
#include <Core/UUID.h>
#include <Scene/Scene.h>
#include <Scene/Entity.h>

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

        static void Entity_GetPosition(UUID entityId, Vector3* position)
        {
            Entity entity = ScriptEngine::GetContext()->GetEntity(entityId);
            *position = entity.GetComponent<TransformComponent>().GetPosition();
        }

        static void Entity_SetPosition(UUID entityId, Vector3* position)
        {
            Entity entity = ScriptEngine::GetContext()->GetEntity(entityId);
            entity.GetComponent<TransformComponent>().SetPosition(*position);
        }
    }


    /*** Register internal call functions ***/

    void ScriptLibrary::RegisterInternalFunctions()
    {
        LV_ADD_INTERNAL_CALL(LogInfo);
        LV_ADD_INTERNAL_CALL(LogTrace);
        LV_ADD_INTERNAL_CALL(LogWarn);
        LV_ADD_INTERNAL_CALL(LogError);
        LV_ADD_INTERNAL_CALL(CrossVec3);
        LV_ADD_INTERNAL_CALL(Entity_GetPosition);
        LV_ADD_INTERNAL_CALL(Entity_SetPosition);
    }

}
