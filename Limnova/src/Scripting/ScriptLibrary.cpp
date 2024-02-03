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

        // Entity ------------------------------------------------------------------------------------------------------------------

        static void Entity_IsValid(UUID entityId, bool *isValid)
        {
            *isValid = ScriptEngine::GetContext()->IsEntity(entityId);
        }

        // -----------------------------------------------------------------------------------------------------------------------------

        static void Entity_HasComponent(UUID entityId, MonoReflectionType* componentType, bool* hasComponent)
        {
            Entity entity = ScriptEngine::GetContext()->GetEntity(entityId);
            LV_CORE_ASSERT(entity != Entity::Null, "Method called from uninitialized script entity!");

            MonoType* managedType = mono_reflection_type_get_type(componentType);
            LV_CORE_ASSERT(ScriptLibrary::GetEntityHasComponentFuncs().contains(managedType), "Component has not been registered!");

            *hasComponent = ScriptLibrary::GetEntityHasComponentFuncs().at(managedType)(entity);
        }

        // Components --------------------------------------------------------------------------------------------------------------

        static void TransformComponent_GetPosition(UUID entityId, Vector3* pPosition)
        {
            Entity entity = ScriptEngine::GetContext()->GetEntity(entityId);
            *pPosition = entity.GetComponent<TransformComponent>().GetPosition();
        }

        // -----------------------------------------------------------------------------------------------------------------------------

        static void TransformComponent_SetPosition(UUID entityId, Vector3* pPosition)
        {
            Entity entity = ScriptEngine::GetContext()->GetEntity(entityId);
            entity.GetComponent<TransformComponent>().SetPosition(*pPosition);
        }

        // Physics ---------------------------------------------------------------------------------------------------------------------

        static void OrbitalPhysics_GetVelocity(UUID entityId, Vector3d* pVelocity)
        {
            Entity entity = ScriptEngine::GetContext()->GetEntity(entityId);
            if (!entity.HasComponent<OrbitalComponent>())
                LV_CORE_WARN("Cannot set thrust on entity ({}) - does not have an orbital component!", entityId);

            *pVelocity = entity.GetComponent<OrbitalComponent>().Object.GetState().Velocity;
        }

        // -----------------------------------------------------------------------------------------------------------------------------

        static void OrbitalPhysics_ComputeLocalAcceleration(UUID entityId, double thrust, double *pLocalAcceleration)
        {
            Entity entity = ScriptEngine::GetContext()->GetEntity(entityId);
            if (!entity.HasComponent<OrbitalComponent>())
                LV_CORE_WARN("Cannot set thrust on entity ({}) - does not have an orbital component!", entityId);

            OrbitalPhysics::ObjectNode objectNode = entity.GetComponent<OrbitalComponent>().Object;
            *pLocalAcceleration = thrust / objectNode.ParentLsp().GetLSpace().MetersPerRadius / objectNode.GetState().Mass;
        }

        // -----------------------------------------------------------------------------------------------------------------------------

        static void OrbitalPhysics_SetThrust(UUID entityId, Vector3d* pThrust)
        {
            Entity entity = ScriptEngine::GetContext()->GetEntity(entityId);
            if (!entity.HasComponent<OrbitalComponent>())
                LV_CORE_WARN("Cannot set thrust on entity ({}) - does not have an orbital component!", entityId);

            entity.GetComponent<OrbitalComponent>().Object.SetContinuousThrust(*pThrust);
        }

        // -----------------------------------------------------------------------------------------------------------------------------

        static void OrbitalPhysics_ComputeSeparation(UUID thisEntityId, UUID otherEntityId, Vector3* direction, double* distance)
        {
            Entity entity = ScriptEngine::GetContext()->GetEntity(thisEntityId);
            Entity otherEntity = ScriptEngine::GetContext()->GetEntity(otherEntityId);

            if (entity && otherEntity)
            {
                if (entity.HasComponent<OrbitalComponent>() && otherEntity.HasComponent<OrbitalComponent>())
                {
                    OrbitalPhysics::ObjectNode thisObjectNode = entity.GetComponent<OrbitalComponent>().Object;
                    Vector3 localSeparation = OrbitalPhysics::ComputeLocalSeparation(thisObjectNode,
                        otherEntity.GetComponent<OrbitalComponent>().Object);

                    float localDistance = sqrtf(localSeparation.SqrMagnitude());
                    *distance = localDistance * thisObjectNode.ParentLsp().GetLSpace().MetersPerRadius;

                    *direction = localSeparation.Normalized();
                }
            }
        }

        // -----------------------------------------------------------------------------------------------------------------------------

        static void OrbitalPhysics_SolveMissileIntercept(UUID missileEntityId, UUID targetEntityId, double thrust, float targetingTolerance,
            uint32_t maxIterations, Vector3 *pIntercept, float *pTimeToIntercept)
        {
            Entity missileEntity = ScriptEngine::GetContext()->GetEntity(missileEntityId);
            Entity targetEntity = ScriptEngine::GetContext()->GetEntity(targetEntityId);

            if (missileEntity && targetEntity)
            {
                if (missileEntity.HasComponent<OrbitalComponent>() && targetEntity.HasComponent<OrbitalComponent>())
                {
                    OrbitalPhysics::ObjectNode missileObjectNode = missileEntity.GetComponent<OrbitalComponent>().Object;
                    OrbitalPhysics::ObjectNode targetObjectNode = targetEntity.GetComponent<OrbitalComponent>().Object;

                    double localMetersPerRadius = missileObjectNode.ParentLsp().GetLSpace().MetersPerRadius;
                    double localThrust = thrust / localMetersPerRadius;
                    float localTolerance = targetingTolerance / localMetersPerRadius;

                    Vector3 localIntercept;
                    OrbitalPhysics::SolveMissileIntercept(missileObjectNode, targetObjectNode, localThrust, localTolerance,
                        localIntercept, *pTimeToIntercept, maxIterations);

                    *pIntercept = localIntercept - missileObjectNode.GetState().Position;
                }
            }
        }

        // -----------------------------------------------------------------------------------------------------------------------------

        static void OrbitalPhysics_ComputeProportionalNavigationAcceleration(UUID missileEntityId, UUID targetEntityId, float proportionalityConstant,
            Vector3d *pProportionalAcceleration)
        {
            Entity missileEntity = ScriptEngine::GetContext()->GetEntity(missileEntityId);
            Entity targetEntity = ScriptEngine::GetContext()->GetEntity(targetEntityId);

            if (missileEntity && targetEntity)
            {
                if (missileEntity.HasComponent<OrbitalComponent>() && targetEntity.HasComponent<OrbitalComponent>())
                {
                    OrbitalPhysics::ObjectNode missileObjectNode = missileEntity.GetComponent<OrbitalComponent>().Object;
                    OrbitalPhysics::ObjectNode targetObjectNode = targetEntity.GetComponent<OrbitalComponent>().Object;

                    double localMetersPerRadius = missileObjectNode.ParentLsp().GetLSpace().MetersPerRadius;

                    *pProportionalAcceleration = OrbitalPhysics::ComputeProportionalNavigationAcceleration(missileObjectNode, targetObjectNode, proportionalityConstant);
                }
            }
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
        LV_SCRIPT_LIBRARY_REGISTER_INTERNAL_CALL(Entity_IsValid);
        LV_SCRIPT_LIBRARY_REGISTER_INTERNAL_CALL(Entity_HasComponent);
        LV_SCRIPT_LIBRARY_REGISTER_INTERNAL_CALL(TransformComponent_GetPosition);
        LV_SCRIPT_LIBRARY_REGISTER_INTERNAL_CALL(TransformComponent_SetPosition);
        LV_SCRIPT_LIBRARY_REGISTER_INTERNAL_CALL(OrbitalPhysics_GetVelocity);
        LV_SCRIPT_LIBRARY_REGISTER_INTERNAL_CALL(OrbitalPhysics_ComputeLocalAcceleration);
        LV_SCRIPT_LIBRARY_REGISTER_INTERNAL_CALL(OrbitalPhysics_SetThrust);
        LV_SCRIPT_LIBRARY_REGISTER_INTERNAL_CALL(OrbitalPhysics_ComputeSeparation);
        LV_SCRIPT_LIBRARY_REGISTER_INTERNAL_CALL(OrbitalPhysics_SolveMissileIntercept);
        LV_SCRIPT_LIBRARY_REGISTER_INTERNAL_CALL(OrbitalPhysics_ComputeProportionalNavigationAcceleration);
    }

    // -----------------------------------------------------------------------------------------------------------------------------

    std::unordered_map<MonoType*, std::function<bool(Entity)>>& ScriptLibrary::GetEntityHasComponentFuncs()
    {
        return s_EntityHasComponentFuncs;
    }

    // -----------------------------------------------------------------------------------------------------------------------------
}
