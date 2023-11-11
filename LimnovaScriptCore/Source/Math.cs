using System;
using System.Runtime.CompilerServices;

namespace Limnova
{

    // Vec3 ------------------------------------------------------------------------------------------------------------------------

    public struct Vec3
    {
        public float X, Y, Z;

        // -------------------------------------------------------------------------------------------------------------------------

        public Vec3(Vec3 other) => (X, Y, Z) = (other.X, other.Y, other.Z);

        public Vec3(float x, float y, float z) => (X, Y, Z) = (x, y, z);

        // -------------------------------------------------------------------------------------------------------------------------

        public static readonly Vec3 Zero = new Vec3(0.0f);

        // -------------------------------------------------------------------------------------------------------------------------

        public Vec3(float scalar)
        {
            X = scalar; Y = scalar; Z = scalar;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        public float SqrMagnitude()
            => (X * X + Y * Y + Z * Z);

        // -------------------------------------------------------------------------------------------------------------------------

        public Vec3 Cross(Vec3 rhs)
            => new Vec3(
                    Y * rhs.Z - Z * rhs.Y,
                    Z * rhs.X - X * rhs.Z,
                    X * rhs.Y - Y * rhs.X
                );

        // -------------------------------------------------------------------------------------------------------------------------

        public Vec3 Normalized()
        {
            float magnitude = (float)Math.Sqrt((double)SqrMagnitude());
            if (magnitude == 0)
                return Vec3.Zero;

            return new Vec3(this / magnitude);
        }

        // Operators ---------------------------------------------------------------------------------------------------------------

        public static Vec3 operator +(Vec3 lhs, Vec3 rhs)
            => new Vec3(lhs.X + rhs.X, lhs.Y + rhs.Y, lhs.Z + rhs.Z);

        // -------------------------------------------------------------------------------------------------------------------------

        public static Vec3 operator -(Vec3 lhs, Vec3 rhs)
            => new Vec3(lhs.X - rhs.X, lhs.Y - rhs.Y, lhs.Z - rhs.Z);

        // -------------------------------------------------------------------------------------------------------------------------

        public static Vec3 operator *(Vec3 vec3, float scalar)
            => new Vec3(vec3.X * scalar, vec3.Y * scalar, vec3.Z * scalar);

        // -------------------------------------------------------------------------------------------------------------------------

        public static Vec3 operator *(float scalar, Vec3 vec3)
            => new Vec3(vec3.X * scalar, vec3.Y * scalar, vec3.Z * scalar);

        // -------------------------------------------------------------------------------------------------------------------------

        public static Vec3 operator /(Vec3 vec3, float scalar)
        {
            if (scalar == 0)
                return Vec3.Zero;

            return new Vec3(vec3.X / scalar, vec3.Y / scalar, vec3.Z / scalar);
        }

        // ToString ----------------------------------------------------------------------------------------------------------------

        public override string ToString()
            => $"{X} {Y} {Z}";

    }

}
