using System;
using System.Runtime.CompilerServices;

namespace Limnova
{

    public static class Native
    {
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void LogInfo(string message);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void LogTrace(string message);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void LogWarn(string message);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void LogError(string message);


        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Vector3_Cross(ref Vec3 lhs, ref Vec3 rhs, out Vec3 res);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Vector3_Normalized(ref Vec3 vec3, out Vec3 res);


        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Entity_GetPosition(ulong entityId, out Vec3 pos);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Entity_SetPosition(ulong entityId, ref Vec3 pos);


        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Input_IsKeyPressed(KeyCode keyCode, out bool isPressed);
    }

}
