#pragma once


namespace Limnova
{
    /* TUserId should be an identifier type for user-defined objects which
    the user will associate with OrbitalPhysics objects (e.g, the type of
    a numeric identifier used for entities/components in the user's ECS).
    It must be freely copyable so it can be stored by value in
    OrbitalPhysics.

    This template system is used so that OrbitalPhysics can be queried 
    about object relationships (e.g, orbital host or satellites)
    and return direct identifiers to the user-objects associated with the
    requested objects. This is as opposed to returning OrbitalPhysics
    object IDs and forcing the user to search its own objects for them.
     */
    template<typename TUserId>
    class OrbitalPhysics
    {
    public:
        OrbitalPhysics() = default;
        ~OrbitalPhysics() = default;

        using TObjectId = uint32_t;
        static constexpr TObjectId Null = std::numeric_limits<TObjectId>::max();
    public:
        /// <summary>
        /// Create an uninitialised orbital physics object.
        /// </summary>
        /// <param name="userId">ID of the user-object to be associated with the created physics object</param>
        /// <returns>ID of the created physics object</returns>
        TObjectId Create(TUserId userId)
        {
            TObjectId newObject = GetEmptyObject();
            m_Objects[newObject].UserId = userId;
            return newObject;
        }

        /// <summary>
        /// Destroy an orbital physics object.
        /// </summary>
        /// <param name="objectId">ID of the physics object to be destroyed</param>
        void Destroy(TObjectId orbitalObjectId)
        {
            // Resolve object dependencies
            RecycleObject(orbitalObjectId);
        }
    private:
        struct Object
        {
            TUserId UserId;

            Object() = default;
            Object(TUserId userId) : UserId(userId) {}
        };
    private:
        std::vector<Object> m_Objects;
        std::unordered_set<TObjectId> m_EmptyObjects;
    private:
        TObjectId GetEmptyObject()
        {
            TObjectId emptyObject;
            if (m_EmptyObjects.empty())
            {
                emptyObject = m_Objects.size();
                m_Objects.emplace_back();
                return emptyObject;
            }
            auto it = m_EmptyObjects.begin();
            emptyObject = *it;
            m_EmptyObjects.erase(it);
            return emptyObject;
        }

        void RecycleObject(TObjectId object)
        {
            // TODO : clear object data members
            m_EmptyObjects.insert(object);
        }
    };

}
