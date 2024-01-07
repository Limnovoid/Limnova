using System;
using System.Runtime.CompilerServices;

namespace Limnova
{

    public class Missile : Entity
    {
        public bool Seek;
        public double EngineThrust;
        public EntityReference Target;

        public override void OnCreate(ulong entityId)
        {
            base.OnCreate(entityId);
        }

        public override void OnUpdate(float dT)
        {
            if (Seek)
            {
                Native.OrbitalPhysics_ComputeSeparation(this.m_Id, Target.m_EntityId, out Vec3 direction, out double distance);

                Vec3d thrust = new Vec3d(direction) * EngineThrust;

                Native.OrbitalPhysics_SetThrust(this.m_Id, ref thrust);
            }
        }
    }

}
