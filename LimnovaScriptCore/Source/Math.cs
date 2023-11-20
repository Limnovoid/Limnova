using System;
using System.Runtime.CompilerServices;
using System.Numerics;

namespace Limnova
{

    // Vec3 ------------------------------------------------------------------------------------------------------------------------

    public struct Vec3<T> where T : INumber<T>
    {
        public T X, Y, Z;

        // -------------------------------------------------------------------------------------------------------------------------

        public Vec3(Vec3<T> other) => (X, Y, Z) = (other.X, other.Y, other.Z);

        public Vec3(T x, T y, T z) => (X, Y, Z) = (x, y, z);

        // -------------------------------------------------------------------------------------------------------------------------

        public static Vec3<T> Zero<T>() where T : unmanaged
            => new Vec3<T>((T)0);

        // -------------------------------------------------------------------------------------------------------------------------

        public Vec3(T scalar)
        {
            X = scalar; Y = scalar; Z = scalar;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        public T SqrMagnitude()
            => (X * X + Y * Y + Z * Z);

        // -------------------------------------------------------------------------------------------------------------------------

        public Vec3<T> Cross(Vec3<T> rhs)
            => new Vec3<T>(
                    Y * rhs.Z - Z * rhs.Y,
                    Z * rhs.X - X * rhs.Z,
                    X * rhs.Y - Y * rhs.X
                );

        // -------------------------------------------------------------------------------------------------------------------------

        public Vec3<T> Normalized()
        {
            T magnitude = (T)Math.Sqrt((double)SqrMagnitude());
            if (magnitude == 0)
                return Vec3<T>.Zero;

            return new Vec3<T>(this / magnitude);
        }

        // Operators ---------------------------------------------------------------------------------------------------------------

        public static Vec3<T> operator +(Vec3<T> lhs, Vec3<T> rhs)
            => new Vec3<T>(lhs.X + rhs.X, lhs.Y + rhs.Y, lhs.Z + rhs.Z);

        // -------------------------------------------------------------------------------------------------------------------------

        public static Vec3<T> operator -(Vec3<T> lhs, Vec3<T> rhs)
            => new Vec3<T>(lhs.X - rhs.X, lhs.Y - rhs.Y, lhs.Z - rhs.Z);

        // -------------------------------------------------------------------------------------------------------------------------

        public static Vec3<T> operator *(Vec3<T> vec3, T scalar)
            => new Vec3<T>(vec3.X * scalar, vec3.Y * scalar, vec3.Z * scalar);

        // -------------------------------------------------------------------------------------------------------------------------

        public static Vec3<T> operator *(T scalar, Vec3<T> vec3)
            => new Vec3<T>(vec3.X * scalar, vec3.Y * scalar, vec3.Z * scalar);

        // -------------------------------------------------------------------------------------------------------------------------

        public static Vec3<T> operator /(Vec3<T> vec3, T scalar)
        {
            if (scalar == 0)
                return Vec3<T>.Zero;

            return new Vec3<T>(vec3.X / scalar, vec3.Y / scalar, vec3.Z / scalar);
        }

        // ToString ----------------------------------------------------------------------------------------------------------------

        public override string ToString()
            => $"{X} {Y} {Z}";

    }

}
