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
        internal extern static void CrossVec3(ref Vec3 lhs, ref Vec3 rhs, out Vec3 res);
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
            Native.CrossVec3(ref this, ref rhs, out Vec3 res);
            return res;
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
    }

}
