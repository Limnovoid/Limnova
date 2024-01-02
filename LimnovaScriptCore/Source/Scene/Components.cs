using System;
using System.Runtime.CompilerServices;

namespace Limnova
{

    public interface Component
    {
        Entity Entity { get; set; }
    }


    public struct TransformComponent : Component
    {
        public Entity Entity { get; set; }

        public Vec3<float> Position
        {
            get
            {
                Native.TransformComponent_GetPosition(Entity.m_Id, out Vec3<float> position);
                return position;
            }
            set
            {
                Native.TransformComponent_SetPosition(Entity.m_Id, ref value);
            }
        }
    }

}
