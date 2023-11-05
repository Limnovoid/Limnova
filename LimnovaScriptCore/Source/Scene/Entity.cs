using System;
using System.Runtime.CompilerServices;

namespace Limnova
{

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
