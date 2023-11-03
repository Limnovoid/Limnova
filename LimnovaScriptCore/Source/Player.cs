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
            Vec3 pos = Position;
            pos.X += 0.1f * dT;
            Position = pos;
        }
    }

}
