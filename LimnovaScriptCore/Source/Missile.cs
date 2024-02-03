using System;
using System.Runtime.CompilerServices;

namespace Limnova
{

    public class Missile : Entity
    {
        public bool             Seek;
        public uint             SeekMax;
        public float            SeekRecompFactor;
        public float            PropConstant;
        public float            PropNavBias;
        public float            TargetingTolerance;
        public double           EngineThrust;
        public EntityReference  Target;
        public EntityReference  TargetingReticle;

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
                    // Accelerate toward intercept with a proportional navigation bias

                    Native.OrbitalPhysics_SolveMissileIntercept(this.m_Id, Target.m_EntityId, EngineThrust, TargetingTolerance, SeekMax,
                        out Vec3 intercept, out float timeToIntercept);

                    SeekTimer -= timeToIntercept * SeekRecompFactor;

                    Native.OrbitalPhysics_ComputeProportionalNavigationAcceleration(this.m_Id, Target.m_EntityId, PropConstant,
                        out Vec3d proportionalAcceleration);

                    Native.OrbitalPhysics_ComputeLocalAcceleration(this.m_Id, EngineThrust, out double localAcceleration);
                    PropNavBias = Limnova.Math<float>.Clamp((float)(Math.Sqrt(proportionalAcceleration.SqrMagnitude()) / localAcceleration), 0.0f, 1.0f);

                    Vec3 thrustDirection = ((1.0f - PropNavBias) * intercept.Normalized()) +
                        (PropNavBias * (Vec3)(proportionalAcceleration.Normalized()));

                    Vec3d thrustVector = new Vec3d(thrustDirection) * EngineThrust;
                    Native.OrbitalPhysics_SetThrust(this.m_Id, ref thrustVector);

                    // Place reticle
                    if (TargetingReticle.IsReferenceValid())
                    {
                        Vec3 targetingReticlePosition = Transform.Position + intercept;

                        TransformComponent targetingReticleTransform = TargetingReticle.GetComponent<TransformComponent>();
                        targetingReticleTransform.Position = targetingReticlePosition;
                    }
                }
            }
        }
    }

}
