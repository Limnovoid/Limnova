using System;
using System.Runtime.CompilerServices;

namespace Limnova
{

    public class Entity
    {
        /* protected internal getter for access by Entity (sub)classes declared in user-defined assemblies */
        public ulong m_Id { get; protected internal set; } = 0;

        // -------------------------------------------------------------------------------------------------------------------------

        public Entity()
        {
            this.m_Id = 0;
        }

        public Entity(ref Entity entity)
        {
            this.m_Id = entity.m_Id;
        }

        public Entity(ulong id)
        {
            this.m_Id = id;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        public static implicit operator Entity(ulong id) => new Entity(id);

        public static implicit operator ulong(Entity entity) => entity.m_Id;

        // -------------------------------------------------------------------------------------------------------------------------

        public virtual void OnCreate(ulong entityId)
        {
            m_Id = entityId;
            Native.LogInfo($"C#.Limnova.Entity.OnCreate({entityId})");
        }

        // -------------------------------------------------------------------------------------------------------------------------

        public virtual void OnUpdate(float dT)
        {
        }

        // -------------------------------------------------------------------------------------------------------------------------

        public bool HasComponent<T>() where T : Component, new()
        {
            Type componentType = typeof(T);
            Native.Entity_HasComponent(m_Id, componentType, out bool hasComponent);
            return hasComponent;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        public T GetComponent<T>() where T : Component, new()
        {
            if (HasComponent<T>())
                return new T() { Entity = this };

            return new T() { Entity = null };
        }

        // Getters for guaranteed components ---------------------------------------------------------------------------------------

        public TransformComponent Transform
        {
            get => new TransformComponent() { Entity = this };
        }

    }

}
