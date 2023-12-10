using System;
using System.Runtime.CompilerServices;

namespace Limnova
{

    public class Player : Entity
    {
        public float Speed;

        public float    MyFloat = 7.0f;
        public double   MyDouble = 7.0;
        public bool     MyBool = true;
        //public byte     MyByte = byte(1);
        //public char     MyChar = 'A';
        public short    MyShort = -7;
        public int      MyInt = -7;
        public long     MyLong = -7;
        public ushort   MyUshort = 7;
        public uint     MyUint = 7;
        public ulong    MyUlong = 7;
        public Vec2     MyVec2 = new Vec2(1.0f, 2.0f);
        public Vec3     MyVec3 = new Vec3(1.0f, 2.0f, 3.0f );
        public Vec3d    MyVec3d = new Vec3d(1.0, 2.0, 3.0);
        public Entity   MyEntity = 0;

        public void OnCreate(ulong entityId)
        {
            base.OnCreate(entityId);
        }

        public void OnUpdate(float dT)
        {
            base.OnUpdate(dT);

            // test
            Vec3 direction = new Vec3();
            if (Input.IsKeyPressed(KeyCode.KEY_A))
                direction.X = -1f;
            else if (Input.IsKeyPressed(KeyCode.KEY_D))
                direction.X = 1f;
            if (Input.IsKeyPressed(KeyCode.KEY_W))
                direction.Y = 1f;
            else if (Input.IsKeyPressed(KeyCode.KEY_S))
                direction.Y = -1f;

            direction = direction.Normalized();

            float distance = Speed * dT;

            TransformComponent transform = Transform;
            Vec3 pos = transform.Position;
            pos += direction * distance;
            transform.Position = pos;

            Native.LogInfo($"Entity ({m_Id}) position: {pos.ToString()}");
        }
    }

}
