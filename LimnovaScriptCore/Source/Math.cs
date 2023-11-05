using System;
using System.Runtime.CompilerServices;

namespace Limnova
{

    // Vec3 ------------------------------------------------------------------------------------------------------------------------

    public struct Vec3
    {
        public float X, Y, Z;

        public Vec3(float x, float y, float z)
        {
            X = x; Y = y; Z = z;
        }

        public Vec3(float scalar)
        {
            X = scalar; Y = scalar; Z = scalar;
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

}
