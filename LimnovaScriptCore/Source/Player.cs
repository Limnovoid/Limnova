using System;
using System.Runtime.CompilerServices;

namespace Limnova
{

    public class Player : Entity
    {
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

            float speed = 0.1f * dT;

            Vec3 pos = Position;
            pos += direction * speed;
            Position = pos;
        }
    }

}
