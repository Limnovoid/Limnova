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


    public struct Vec3
    {
        public float X, Y, Z;

        public Vec3(float x, float y, float z)
        {
            X = x; Y = y; Z = z;
        }

        public Vec3 Cross(Vec3 rhs)
        {
            Native.Vector3_Cross(ref this, ref rhs, out Vec3 res);
            return res;
        }

        public Vec3 Normalized()
        {
            Native.Vector3_Normalized(ref this, out Vec3 res);
            return res;
        }

        // Operators ---------------------------------------------------------------------------------------------------------------

        public static Vec3 operator +(Vec3 lhs, Vec3 rhs)
        {
            return new Vec3(lhs.X + rhs.X, lhs.Y + rhs.Y, lhs.Z + rhs.Z);
        }

        public static Vec3 operator -(Vec3 lhs, Vec3 rhs)
        {
            return new Vec3(lhs.X - rhs.X, lhs.Y - rhs.Y, lhs.Z - rhs.Z);
        }

        public static Vec3 operator *(Vec3 vec3, float scalar)
        {
            return new Vec3(vec3.X * scalar, vec3.Y * scalar, vec3.Z * scalar);
        }

        public static Vec3 operator *(float scalar, Vec3 vec3)
        {
            return new Vec3(vec3.X * scalar, vec3.Y * scalar, vec3.Z * scalar);
        }

    }


    public class Main
    {
        public float FloatVar { get; set; }

        public Main()
        {
            Native.LogInfo("C#.Limnova.Main.Main()");
            Native.LogTrace("C#.Limnova.Main.Main()");
            Native.LogWarn("C#.Limnova.Main.Main()");
            Native.LogError("C#.Limnova.Main.Main()");

            //Vec3 vec1 = new Vec3(0, 1, 2);
            //InternalCalls.Native_PrintVec3(ref vec1);
            //Vec3 vec2 = new Vec3(0, 1,-2);
            //InternalCalls.Native_PrintVec3(ref vec2);
            //Vec3 v1Xv2 = vec1.Cross(vec2);
            //InternalCalls.Native_PrintVec3(ref v1Xv2);
        }

        public void PrintVec3(float x, float y, float z)
        {
            Native.LogTrace($"PrintVec3: ({x}, {y}, {z})");
        }
    }


    public class Entity
    {
        private ulong EntityId;

        public void OnCreate(ulong entityId)
        {
            EntityId = entityId;
            Native.LogInfo($"C#.Limnova.Entity.OnCreate({entityId})");
        }

        public void OnUpdate(float dT)
        {
        }

        public Vec3 Position
        {
            get
            {
                Native.Entity_GetPosition(EntityId, out Vec3 position);
                return position;
            }
            set
            {
                Native.Entity_SetPosition(EntityId, ref value);
            }
        }
    }

}
