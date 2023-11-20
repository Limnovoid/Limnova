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
            Vec3<float> direction = new Vec3<float>();
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

            //TransformComponent transform = GetComponent<TransformComponent>();
            //if (transform != null)
            //{
            //    Vec3 pos = transform.Position;
            //    pos += direction * speed;
            //    transform.Position = pos;
            //}

            TransformComponent transform = Transform;
            Vec3<float> pos = transform.Position;
            pos += direction * speed;
            transform.Position = pos;

            Native.LogInfo($"Entity ({m_Id}) position: {pos.ToString()}");
        }
    }

}
