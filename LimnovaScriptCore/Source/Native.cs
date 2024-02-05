using System;
using System.Runtime.CompilerServices;

namespace Limnova
{

    public static class Native
    {

        // Logging -----------------------------------------------------------------------------------------------------------------

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void LogInfo(string message);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void LogTrace(string message);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void LogWarn(string message);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void LogError(string message);

        // Input -------------------------------------------------------------------------------------------------------------------

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Input_IsKeyPressed(KeyCode keyCode, out bool isPressed);

        // Entity ------------------------------------------------------------------------------------------------------------------

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Entity_IsValid(ulong entityId, out bool isValid);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Entity_HasComponent(ulong entityId, Type componentType, out bool hasComponent);

        // Components --------------------------------------------------------------------------------------------------------------

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void TransformComponent_GetPosition(ulong entityId, out Vec3 position);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void TransformComponent_SetPosition(ulong entityId, ref Vec3 position);

        // Physics -----------------------------------------------------------------------------------------------------------------

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void OrbitalPhysics_GetVelocity(ulong entityId, out Vec3d velocity);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void OrbitalPhysics_ComputeLocalAcceleration(ulong entityId, double thrust, out double localAcceleration);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void OrbitalPhysics_SetThrust(ulong entityId, ref Vec3d thrust);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void OrbitalPhysics_ComputeSeparation(ulong thisEntityId, ulong otherEntityId, out Vec3 direction,
            out double distance);

        /// <summary>
        /// Solve for point at which a 'missile' entity intercepts a 'target' entity, assuming constant thrust.
        /// Returned value is relative to the missile entity.
        /// </summary>
        /// <param name="thisEntityId">ID of missile entity</param>
        /// <param name="otherEntityId">ID of target entity</param>
        /// <param name="thrust">Magnitude of missile's engine thrust</param>
        /// <param name="targetingTolerance">Permissible difference between solved intercept and actual intercept</param>
        /// <param name="intercept">Storage for return value</param>
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void OrbitalPhysics_SolveMissileIntercept(ulong thisEntityId, ulong otherEntityId, double thrust,
            float targetingTolerance, uint maxIterations, out Vec3 intercept, out float timeToIntercept);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void OrbitalPhysics_ComputeProportionalNavigationAcceleration(ulong thisEntityId, ulong otherEntityId,
            float proportionalityConstant, out Vec3d proportionalAcceleration);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void OrbitalPhysics_SolveMissileInterceptVector(ulong missileEntityId, ulong targetEntityId, double thrust,
            float targetingTolerance, uint maxIterations, float proportionalityConstant, out Vec3 interceptVector, out Vec3 intercept,
            out float timeToIntercept);
    }

}
