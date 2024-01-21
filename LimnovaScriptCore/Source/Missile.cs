using System;
using System.Runtime.CompilerServices;

namespace Limnova
{

    public class Missile : Entity
    {
        public bool             Seek;
        public float            TargetingTolerance;
        public double           EngineThrust;
        public EntityReference  Target;
        public EntityReference  TargetingReticle;

        public override void OnCreate(ulong entityId)
        {
            base.OnCreate(entityId);
        }

        public override void OnUpdate(float dT)
        {
            if (Seek)
            {
                Native.OrbitalPhysics_SolveMissileIntercept(this.m_Id, Target.m_EntityId, EngineThrust, TargetingTolerance,
                    out Vec3 intercept);

                Vec3 interceptDirection = intercept.Normalized();
                Vec3d thrustVector = new Vec3d(interceptDirection) * EngineThrust;

                Native.OrbitalPhysics_SetThrust(this.m_Id, ref thrustVector);

                // Place targeting reticle
                if (TargetingReticle.IsReferenceValid())
                {
                    Vec3 targetingReticlePosition = Transform.Position + intercept;

                    TransformComponent targetingReticleTransform = TargetingReticle.GetComponent<TransformComponent>();
                    targetingReticleTransform.Position = targetingReticlePosition;
                }
                else
                {
                    Native.LogError("TargetingReticle not found");
                }
            }
        }
    }

}
