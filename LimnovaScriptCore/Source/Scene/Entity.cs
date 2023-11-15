using System;
using System.Runtime.CompilerServices;

namespace Limnova
{

    public class Entity
    {
        public ulong m_Id { get; private set; }

        // -------------------------------------------------------------------------------------------------------------------------

        protected Entity()
        {
            m_Id = 0;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        public void OnCreate(ulong entityId)
        {
            m_Id = entityId;
            Native.LogInfo($"C#.Limnova.Entity.OnCreate({entityId})");
        }

        // -------------------------------------------------------------------------------------------------------------------------

        public void OnUpdate(float dT)
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
