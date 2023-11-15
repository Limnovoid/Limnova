#include "ScriptLibrary.h"

#include "ScriptEngine.h"

#include <mono/metadata/object.h>
#include <mono/metadata/reflection.h>

#include <Math/Math.h>

#include <Core/UUID.h>
#include <Core/Input.h>
#include <Core/KeyCodes.h>

#include <Scene/Scene.h>
#include <Scene/Entity.h>

#define LV_SCRIPT_LIBRARY_REGISTER_INTERNAL_CALL(func) mono_add_internal_call("Limnova.Native::" #func, InternalCall::func)

namespace Limnova
{
    // -----------------------------------------------------------------------------------------------------------------------------

    static std::string ToString(MonoString* monoStr)
    {
        char* cStr = mono_string_to_utf8(monoStr);
        std::string str = { cStr };
        mono_free(cStr);
        return str;
    }

    // -----------------------------------------------------------------------------------------------------------------------------

    namespace InternalCall
    {
        // Logging -----------------------------------------------------------------------------------------------------------------

        static void LogInfo(MonoString* message) { LV_INFO(ToString(message)); }
        static void LogTrace(MonoString* message) { LV_TRACE(ToString(message)); }
        static void LogWarn(MonoString* message) { LV_WARN(ToString(message)); }
        static void LogError(MonoString* message) { LV_ERROR(ToString(message)); }

        // Input -------------------------------------------------------------------------------------------------------------------

        static void Input_IsKeyPressed(KeyCode keyCode, bool* isPressed)
        {
            *isPressed = Input::IsKeyPressed(keyCode);
        }

        // Components --------------------------------------------------------------------------------------------------------------

        static void Entity_HasComponent(UUID entityId, MonoReflectionType* componentType, bool* hasComponent)
        {
            Entity entity = ScriptEngine::GetContext()->GetEntity(entityId);
            LV_CORE_ASSERT(entity != Entity::Null, "Method called from uninitialized script entity!");

            MonoType* managedType = mono_reflection_type_get_type(componentType);
            LV_CORE_ASSERT(ScriptLibrary::GetEntityHasComponentFuncs().contains(managedType), "Component has not been registered!");
            *hasComponent = ScriptLibrary::GetEntityHasComponentFuncs().at(managedType)(entity);
        }

        // -----------------------------------------------------------------------------------------------------------------------------

        static void TransformComponent_GetPosition(UUID entityId, Vector3* position)
        {
            Entity entity = ScriptEngine::GetContext()->GetEntity(entityId);
            *position = entity.GetComponent<TransformComponent>().GetPosition();
        }

        // -----------------------------------------------------------------------------------------------------------------------------

        static void TransformComponent_SetPosition(UUID entityId, Vector3* position)
        {
            Entity entity = ScriptEngine::GetContext()->GetEntity(entityId);
            entity.GetComponent<TransformComponent>().SetPosition(*position);
        }

    }

    // Scripting registration ------------------------------------------------------------------------------------------------------

    using ScriptAccessibleComponents = ComponentGroup<TransformComponent, CameraComponent,
        SpriteRendererComponent, BillboardSpriteRendererComponent, CircleRendererComponent, BillboardCircleRendererComponent,
        EllipseRendererComponent, OrbitalComponent>;

    template<typename... TComponent>
    static void RegisterComponent(ScriptLibrary::EntityHasComponentFuncMap& funcMap, MonoImage* coreAssemblyImage)
    {
        ([&funcMap, coreAssemblyImage]()
        {
            std::string_view typeName = typeid(TComponent).name();
            size_t posComponentName = typeName.find_last_of(':');
            std::string_view componentName = typeName.substr(posComponentName + 1);
            std::string csComponentName = fmt::format("Limnova.{}", componentName);

            MonoType* managedType = mono_reflection_type_from_name(csComponentName.data(), coreAssemblyImage);
            if (!managedType)
            {
                LV_CORE_ERROR("Failed to register component '{}'!", csComponentName);
                return;
            }
            funcMap[managedType] = [](Entity entity) -> bool { return entity.HasComponent<TComponent>(); };
        }(), ...);
    }

    template<typename... TComponent>
    static void RegisterComponents(ComponentGroup<TComponent...>, ScriptLibrary::EntityHasComponentFuncMap& funcMap, MonoImage* coreAssemblyImage)
    {
        RegisterComponent<TComponent...>(funcMap, coreAssemblyImage);
    }

    // -----------------------------------------------------------------------------------------------------------------------------
    // -----------------------------------------------------------------------------------------------------------------------------

    ScriptLibrary::EntityHasComponentFuncMap ScriptLibrary::s_EntityHasComponentFuncs = {};

    // -----------------------------------------------------------------------------------------------------------------------------

    void ScriptLibrary::RegisterComponentTypes(MonoImage* coreAssemblyImage)
    {
        RegisterComponents(ScriptAccessibleComponents{}, s_EntityHasComponentFuncs, coreAssemblyImage);
    }

    // -----------------------------------------------------------------------------------------------------------------------------

    void ScriptLibrary::RegisterInternalCalls()
    {
        LV_SCRIPT_LIBRARY_REGISTER_INTERNAL_CALL(LogInfo);
        LV_SCRIPT_LIBRARY_REGISTER_INTERNAL_CALL(LogTrace);
        LV_SCRIPT_LIBRARY_REGISTER_INTERNAL_CALL(LogWarn);
        LV_SCRIPT_LIBRARY_REGISTER_INTERNAL_CALL(LogError);
        LV_SCRIPT_LIBRARY_REGISTER_INTERNAL_CALL(Input_IsKeyPressed);
        LV_SCRIPT_LIBRARY_REGISTER_INTERNAL_CALL(Entity_HasComponent);
        LV_SCRIPT_LIBRARY_REGISTER_INTERNAL_CALL(TransformComponent_GetPosition);
        LV_SCRIPT_LIBRARY_REGISTER_INTERNAL_CALL(TransformComponent_SetPosition);
    }

    // -----------------------------------------------------------------------------------------------------------------------------

    std::unordered_map<MonoType*, std::function<bool(Entity)>>& ScriptLibrary::GetEntityHasComponentFuncs()
    {
        return s_EntityHasComponentFuncs;
    }

    // -----------------------------------------------------------------------------------------------------------------------------
}
