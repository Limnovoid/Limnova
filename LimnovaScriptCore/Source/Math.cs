using System;
using System.Runtime.CompilerServices;

namespace Limnova
{

    // Vec2 ------------------------------------------------------------------------------------------------------------------------

    public struct Vec2
    {
        public float X, Y;

        // -------------------------------------------------------------------------------------------------------------------------

        public Vec2(Vec2 other) => (X, Y) = (other.X, other.Y);

        public Vec2(float x, float y) => (X, Y) = (x, y);

        public Vec2(float value) => (X, Y) = (value, value);

        // -------------------------------------------------------------------------------------------------------------------------

        public static readonly Vec2 Zero = new Vec2(0.0f);

        // -------------------------------------------------------------------------------------------------------------------------

        public float SqrMagnitude()
            => (X * X + Y * Y);

        // -------------------------------------------------------------------------------------------------------------------------

        public Vec2 Normalized()
        {
            float magnitude = (float)Math.Sqrt((double)SqrMagnitude());
            if (magnitude == 0)
                return Vec2.Zero;

            return new Vec2(this / magnitude);
        }

        // Operators ---------------------------------------------------------------------------------------------------------------

        public static Vec2 operator +(Vec2 lhs, Vec2 rhs)
            => new Vec2(lhs.X + rhs.X, lhs.Y + rhs.Y);

        // -------------------------------------------------------------------------------------------------------------------------

        public static Vec2 operator -(Vec2 lhs, Vec2 rhs)
            => new Vec2(lhs.X - rhs.X, lhs.Y - rhs.Y);

        // -------------------------------------------------------------------------------------------------------------------------

        public static Vec2 operator *(Vec2 vec2, float scalar)
            => new Vec2(vec2.X * scalar, vec2.Y * scalar);

        // -------------------------------------------------------------------------------------------------------------------------

        public static Vec2 operator *(float scalar, Vec2 vec2)
            => new Vec2(vec2.X * scalar, vec2.Y * scalar);

        // -------------------------------------------------------------------------------------------------------------------------

        public static Vec2 operator /(Vec2 vec2, float scalar)
        {
            if (scalar == 0)
                return Vec2.Zero;

            return new Vec2(vec2.X / scalar, vec2.Y / scalar);
        }

        // ToString ----------------------------------------------------------------------------------------------------------------

        public override string ToString()
            => $"{X} {Y}";

    }

    // Vec3 ------------------------------------------------------------------------------------------------------------------------

    public struct Vec3
    {
        public float X, Y, Z;

        // -------------------------------------------------------------------------------------------------------------------------

        public Vec3(Vec3 other) => (X, Y, Z) = (other.X, other.Y, other.Z);

        public Vec3(float x, float y, float z) => (X, Y, Z) = (x, y, z);

        public Vec3(float value) => (X, Y, Z) = (value, value, value);

        public Vec3(Vec2 vec2) => (X, Y, Z) = (vec2.X, vec2.Y, 0.0f);

        // -------------------------------------------------------------------------------------------------------------------------

        public static readonly Vec3 Zero = new Vec3(0.0f);

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

    // Vec3d -----------------------------------------------------------------------------------------------------------------------

    public struct Vec3d
    {
        public double X, Y, Z;

        // -------------------------------------------------------------------------------------------------------------------------

        public Vec3d(Vec3d other) => (X, Y, Z) = (other.X, other.Y, other.Z);

        public Vec3d(double x, double y, double z) => (X, Y, Z) = (x, y, z);

        public Vec3d(double value) => (X, Y, Z) = (value, value, value);

        public Vec3d(Vec3 vec3) => (X, Y, Z) = ((double)vec3.X, (double)vec3.Y, (double)vec3.Z);

        public Vec3d(Vec2 vec2) => (X, Y, Z) = ((double)vec2.X, (double)vec2.Y, 0.0);

        // -------------------------------------------------------------------------------------------------------------------------

        public static readonly Vec3d Zero = new Vec3d(0.0f);

        // -------------------------------------------------------------------------------------------------------------------------

        public double SqrMagnitude()
            => (X * X + Y * Y + Z * Z);

        // -------------------------------------------------------------------------------------------------------------------------

        public Vec3d Cross(Vec3d rhs)
            => new Vec3d(
                    Y * rhs.Z - Z * rhs.Y,
                    Z * rhs.X - X * rhs.Z,
                    X * rhs.Y - Y * rhs.X
                );

        // -------------------------------------------------------------------------------------------------------------------------

        public Vec3d Normalized()
        {
            double magnitude = (double)Math.Sqrt((double)SqrMagnitude());
            if (magnitude == 0)
                return Vec3d.Zero;

            return new Vec3d(this / magnitude);
        }

        // Operators ---------------------------------------------------------------------------------------------------------------

        public static Vec3d operator +(Vec3d lhs, Vec3d rhs)
            => new Vec3d(lhs.X + rhs.X, lhs.Y + rhs.Y, lhs.Z + rhs.Z);

        // -------------------------------------------------------------------------------------------------------------------------

        public static Vec3d operator -(Vec3d lhs, Vec3d rhs)
            => new Vec3d(lhs.X - rhs.X, lhs.Y - rhs.Y, lhs.Z - rhs.Z);

        // -------------------------------------------------------------------------------------------------------------------------

        public static Vec3d operator *(Vec3d vec3, double scalar)
            => new Vec3d(vec3.X * scalar, vec3.Y * scalar, vec3.Z * scalar);

        // -------------------------------------------------------------------------------------------------------------------------

        public static Vec3d operator *(double scalar, Vec3d vec3)
            => new Vec3d(vec3.X * scalar, vec3.Y * scalar, vec3.Z * scalar);

        // -------------------------------------------------------------------------------------------------------------------------

        public static Vec3d operator /(Vec3d vec3, double scalar)
        {
            if (scalar == 0)
                return Vec3d.Zero;

            return new Vec3d(vec3.X / scalar, vec3.Y / scalar, vec3.Z / scalar);
        }

        // ToString ----------------------------------------------------------------------------------------------------------------

        public override string ToString()
            => $"{X} {Y} {Z}";

    }

}
