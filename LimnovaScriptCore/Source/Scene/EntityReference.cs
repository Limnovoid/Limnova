using System;

namespace Limnova
{

    public struct EntityReference
    {
        public ulong m_EntityId { get; set; } = 0;

        // -------------------------------------------------------------------------------------------------------------------------

        public EntityReference(ulong entityId)
        {
            this.m_EntityId = entityId;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        public bool IsReferenceValid()
        {
            Native.Entity_IsValid(m_EntityId, out bool isValid);
            return isValid;
        }

        public static explicit operator bool(EntityReference entityReference) { return entityReference.IsReferenceValid(); }

        // -------------------------------------------------------------------------------------------------------------------------

        public bool HasComponent<T>() where T : Component, new()
        {
            if (!IsReferenceValid())
                return false;

            Type componentType = typeof(T);
            Native.Entity_HasComponent(m_EntityId, componentType, out bool hasComponent);
            return hasComponent;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        public T GetComponent<T>() where T : Component, new()
        {
            if (HasComponent<T>())
                return new T() { Entity = new Entity(m_EntityId) };

            return new T() { Entity = new Entity() };
        }
    }

}
