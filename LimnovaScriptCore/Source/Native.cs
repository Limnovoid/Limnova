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
        internal extern static void TransformComponent_GetPosition(ulong entityId, out Vec3 pos);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void TransformComponent_SetPosition(ulong entityId, ref Vec3 pos);

        // Physics -----------------------------------------------------------------------------------------------------------------

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void OrbitalPhysics_SetThrust(ulong entityId, ref Vec3d thrust);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void OrbitalPhysics_ComputeSeparation(ulong thisEntityId, ulong otherEntityId, out Vec3 direction,
            out double distance);
    }

}
