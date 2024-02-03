using System;
using System.Runtime.CompilerServices;

namespace Limnova
{

    public class Missile : Entity
    {
        public bool             Seek;
        public uint             SeekMaxIterations;
        public float            TargetingTolerance;
        public double           EngineThrust;
        public EntityReference  Target;
        public EntityReference  TargetingReticle;
        public EntityReference  AimingReticle;

        float SeekTimer = 0.0f;

        public override void OnCreate(ulong entityId)
        {
            base.OnCreate(entityId);
        }

        public override void OnUpdate(float dT)
        {
            if (!Seek)
            {
                SeekTimer = 0.0f;
            }
            else
            {
                SeekTimer += dT;
                if (SeekTimer > 0.0f)
                {
                    Native.OrbitalPhysics_SolveMissileIntercept(this.m_Id, Target.m_EntityId, EngineThrust, TargetingTolerance, SeekMaxIterations,
                        out Vec3 intercept, out float timeToIntercept);

                    SeekTimer -= timeToIntercept * 0.3f;

                    Native.OrbitalPhysics_GetVelocity(this.m_Id, out Vec3d velocity);
                    Vec3 targetDeltaV = intercept - ((Vec3)(velocity * (double)timeToIntercept));

                    Vec3 interceptDirection = (intercept + targetDeltaV).Normalized();
                    Vec3d thrustVector = new Vec3d(interceptDirection) * EngineThrust;

                    Native.OrbitalPhysics_SetThrust(this.m_Id, ref thrustVector);

                    // Place reticles
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
                    if (AimingReticle.IsReferenceValid())
                    {
                        Vec3 aimingReticlePosition = Transform.Position + intercept + targetDeltaV;

                        TransformComponent aimingReticleTransform = AimingReticle.GetComponent<TransformComponent>();
                        aimingReticleTransform.Position = aimingReticlePosition;
                    }
                    else
                    {
                        Native.LogError("TargetingReticle not found");
                    }
                }
            }
        }
    }

}
