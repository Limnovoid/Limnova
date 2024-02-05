using System;
using System.Runtime.CompilerServices;

namespace Limnova
{

    public class Missile : Entity
    {
        public bool             Seek;
        public uint             SeekMaxIter;
        public float            SeekRecompFactor;
        public float            PropConstant;
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
                    Native.OrbitalPhysics_SolveMissileInterceptVector(this.m_Id, Target.m_EntityId, EngineThrust, TargetingTolerance,
                        SeekMaxIter, PropConstant, out Vec3 interceptVector, out Vec3 intercept, out float timeToIntercept);

                    Vec3d thrustVector = new Vec3d(interceptVector) * EngineThrust;
                    Native.OrbitalPhysics_SetThrust(this.m_Id, ref thrustVector);

                    // Place reticle
                    if (TargetingReticle.IsReferenceValid())
                    {
                        TransformComponent targetingReticleTransform = TargetingReticle.GetComponent<TransformComponent>();
                        targetingReticleTransform.Position = intercept;
                    }

                    SeekTimer -= timeToIntercept * SeekRecompFactor;
                }
            }
        }
    }

}
