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

        // Components --------------------------------------------------------------------------------------------------------------

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Entity_HasComponent(ulong entityId, Type componentType, out bool hasComponent);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void TransformComponent_GetPosition(ulong entityId, out Vec3 pos);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void TransformComponent_SetPosition(ulong entityId, ref Vec3 pos);

    }

}
