#pragma once

#include <Math/Math.h>


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
        OrbitalPhysics()
        {
            m_Objects[m_RootObject].Validity = Validity::InvalidMass; /* Object::Validity is initialised to InvalidParent but that is meaningless for the root object (which cannot be parented) */
            auto& rootspace = m_LocalSpaces.Add(m_RootObject);
            rootspace.Radius = 1.f;
            rootspace.Influencing = true;
        }
        ~OrbitalPhysics()
        {
            // Estimating maximum object allocation for optimising vectors -> arrays
            LV_CORE_INFO("OrbitalPhysics final object count: {0}", m_Objects.size());
        }

        using TObjectId = uint32_t;
        static constexpr TObjectId Null = std::numeric_limits<TObjectId>::max();
    private:
        static constexpr double kGravitational = 6.6743e-11;
        static constexpr float kDefaultLocalSpaceRadius = 0.1f;
    private:
        using TAttrId = uint32_t;

        template<typename Attr>
        class AttributeStorage
        {
        private:
            std::vector<Attr> m_Attributes;
            std::unordered_set<TAttrId> m_Empties;
            std::unordered_map<TObjectId, TAttrId> m_ObjectToAttr;
        public:
            bool Has(TObjectId object)
            {
                return m_ObjectToAttr.find(object) != m_ObjectToAttr.end();
            }

            Attr& Add(TObjectId object)
            {
                LV_CORE_ASSERT(m_ObjectToAttr.find(object) == m_ObjectToAttr.end(), "Object already has attribute!");
                TAttrId attr = GetEmpty();
                m_ObjectToAttr[object] = attr;
                return m_Attributes[attr];
            }

            Attr& Get(TObjectId object)
            {
                LV_CORE_ASSERT(m_ObjectToAttr.find(object) != m_ObjectToAttr.end(), "Object is missing requested attribute!");
                return m_Attributes[m_ObjectToAttr[object]];
            }

            Attr& GetOrAdd(TObjectId object)
            {
                return m_ObjectToAttr.find(object) == m_ObjectToAttr.end() ?
                    Add(object) : Get(object);
            }

            void Remove(TObjectId object)
            {
                LV_CORE_ASSERT(m_ObjectToAttr.find(object) != m_ObjectToAttr.end(), "Object does not have the attribute to remove!");
                Recycle(m_ObjectToAttr[object]);
                m_ObjectToAttr.erase(object);
            }

            void TryRemove(TObjectId object)
            {
                if (m_ObjectToAttr.find(object) != m_ObjectToAttr.end())
                {
                    Recycle(m_ObjectToAttr[object]);
                    m_ObjectToAttr.erase(object);
                }
            }
        public:
            const Attr& operator[](TObjectId object)
            {
                LV_CORE_ASSERT(m_ObjectToAttr.find(object) != m_ObjectToAttr.end(), "Object is missing requested attribute!");
                return m_Attributes[m_ObjectToAttr[object]];
            }
        private:
            TAttrId GetEmpty()
            {
                TAttrId emptyAttr;
                if (m_Empties.empty())
                {
                    emptyAttr = m_Attributes.size();
                    m_Attributes.emplace_back();
                    return emptyAttr;
                }
                auto it = m_Empties.begin();
                emptyAttr = *it;
                m_Empties.erase(it);
                return emptyAttr;
            }

            void Recycle(TAttrId attributes)
            {
                m_Empties.insert(attributes);
            }
        };
    public:
        enum class Validity
        {
            InvalidParent   = 0,
            InvalidMass     = 1,
            InvalidPosition = 2,
            Valid           = 100
        };
    private:
        /*** Objects ***/

        struct State
        {
            /* Compulsory attribute - all physics objects are expected to have a physics state */
            double Mass = 0.0;
            Vector3 Position = { 0.f, 0.f, 0.f };
            Vector3 Velocity = { 0.f, 0.f, 0.f };
        };

        struct Object
        {
            TUserId UserId = TUserId();

            TObjectId Parent = Null;
            TObjectId PrevSibling = Null, NextSibling = Null;

            Validity Validity = Validity::InvalidParent;
            State State;

            Object() = default;
            Object(TUserId userId) : UserId(userId) {}
        };


        /*** Attributes ***/

        struct Elements
        {
            double Grav = 0.f;
            Vector3 H = { 0.f, 0.f, 0.f };
        };

        struct LocalSpace
        {
            float Radius = 0.f; /* Measured in parent's influence */
            float MetersPerRadius = 0.f;

            bool Influencing = false;

            TObjectId FirstChild = Null;
        };
    private:
        /*** Simulation resources ***/

        TObjectId m_RootObject = 0;
        std::vector<Object> m_Objects = { Object() }; /* Initialised with root object */
        std::unordered_set<TObjectId> m_EmptyObjects;

        AttributeStorage<Elements> m_Elements;
        AttributeStorage<LocalSpace> m_LocalSpaces;
    private:
        /*** Resource helpers ***/

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
            LV_CORE_ASSERT(object != 0, "Cannot recycle the root object!");
            m_Objects[object].Parent = Null;
            m_Objects[object].Validity = Validity::InvalidParent;
            m_EmptyObjects.insert(object);
        }

        // Inserts object into object hierarchy
        void AttachObject(TObjectId object, TObjectId parent)
        {
            auto& obj = m_Objects[object];
            auto& ls = m_LocalSpaces.Get(parent);

            // Connect to parent
            obj.Parent = parent;
            if (ls.FirstChild == Null)
            {
                ls.FirstChild = object;
            }
            else
            {
                // Connect to siblings
                auto& next = m_Objects[ls.FirstChild];
                if (next.PrevSibling == Null)
                {
                    // Only one sibling
                    obj.PrevSibling = obj.NextSibling = ls.FirstChild;
                    next.PrevSibling = next.NextSibling = object;
                }
                else
                {
                    // More than one sibling
                    auto& prev = m_Objects[next.PrevSibling];

                    obj.NextSibling = ls.FirstChild;
                    obj.PrevSibling = next.PrevSibling;

                    next.PrevSibling = object;
                    prev.NextSibling = object;
                }
            }
        }

        // Removes object from object hierarchy
        void DetachObject(TObjectId object)
        {
            auto& obj = m_Objects[object];
            auto& ls = m_LocalSpaces.Get(obj.Parent);

            // Disconnect from parent
            if (ls.FirstChild == object)
            {
                /* No need to check if this entity has siblings - NextSibling is the null entity in this case */
                ls.FirstChild = obj.NextSibling;
            }
            obj.Parent = Null;

            // Disconnect from siblings
            if (obj.NextSibling != Null)
            {
                auto& next = m_Objects[obj.NextSibling];
                if (obj.NextSibling == obj.PrevSibling)
                {
                    // Only one sibling
                    next.NextSibling = next.PrevSibling = Null;
                }
                else
                {
                    // More than one sibling
                    auto& prev = m_Objects[obj.PrevSibling];

                    next.PrevSibling = obj.PrevSibling;
                    prev.NextSibling = obj.NextSibling;
                }

                obj.NextSibling = obj.PrevSibling = Null;
            }
        }


        /*** Simulation helpers ***/

        bool ValidPosition(TObjectId object)
        {
            static constexpr float kEscapeDistance = 1.f;

            return m_Objects[object].State.Position.SqrMagnitude() < kEscapeDistance;
                /* TODO - check for influence overlaps */;
        }

        bool ValidMass(TObjectId object)
        {
            static constexpr double kMaxCOG = 1e-4; /* Maximum offset for shared centre of gravity */

            bool hasValidMass = m_Objects[object].State.Mass > 0.0;
            if (object == m_RootObject)
            {
                // TODO - min/max root mass ?
            }
            else
            {
                hasValidMass = hasValidMass && kMaxCOG > m_Objects[object].State.Mass / (m_Objects[object].State.Mass + m_Objects[m_Objects[object].Parent].State.Mass);
            }
            return hasValidMass;
        }

        bool ValidParent(TObjectId object)
        {
            return m_LocalSpaces.Has(m_Objects[object].Parent) || m_Objects[object].Parent == m_RootObject || object == m_RootObject;
        }

        void ComputeValidity(TObjectId object)
        {
            // TODO : #ifdef LV_DEBUG ??? is validity needed in release?

            Validity validity = Validity::Valid;
            if (!ValidParent(object)) {
                validity = Validity::InvalidParent;
            }
            else if (!ValidMass(object)) {
                validity = Validity::InvalidMass;
            }
            else if (!ValidPosition(object)) {
                validity = Validity::InvalidPosition;
            }
            m_Objects[object].Validity = validity;
        }

        void ComputeElements(TObjectId object)
        {
            LV_CORE_ASSERT(object != m_RootObject, "Cannot compute elements on root object!")

            auto& obj = m_Objects[object];
            auto& state = obj.State;
            auto& elems = m_Elements.Get(object);
            auto& par = m_Objects[obj.Parent];

            LV_CORE_ASSERT(obj.Validity == Validity::Valid, "Cannot compute elements on an invalid object!");

            elems.Grav = kGravitational * par.State.Mass * pow(m_LocalSpaces.Get(obj.Parent).MetersPerRadius, -3.0);

            // TODO ...
        }

        float CircularOrbitSpeed(TObjectId object)
        {
            auto& obj = m_Objects[object];
            auto& par = m_Objects[obj.Parent];

            /* ||V_circular|| = sqrt(||r|| * mu), where mu is the gravitational parameter of the orbit */
            return sqrtf(obj.State.Position.SqrMagnitude()) * kGravitational * par.State.Mass * pow(m_LocalSpaces.Get(obj.Parent).MetersPerRadius, -3.0);
        }
    public:
        /*** Usage ***/

        /// <summary>
        /// Assign the orbital physics root object a user object and return the physics object's ID.
        /// Usage example: assign the physics root to an entity in the user's game scene (potentially the root
        /// entity of the scene itself) and use that entity to represent the orbital system's primary object,
        /// e.g, using light and mesh components to display the entity as a star.
        /// </summary>
        /// <typeparam name="TUserId">ID of the user-object to be associated with the root physics object</typeparam>
        TObjectId AssignRoot(TUserId userRootId)
        {
            m_Objects[m_RootObject].UserId = userRootId;
            return m_RootObject;
        }

        /// <summary>
        /// Sets scaling of root orbital space.
        /// Scaling is in meters per simulation length-unit.
        /// E.g, position vector with magnitude 1 in the root orbital space has a simulated magnitude equal to the root scaling.
        /// </summary>
        /// <param name="meters"></param>
        void SetRootScaling(double meters)
        {
            m_LocalSpaces[m_RootObject].MetersPerRadius = meters;

            // TODO - recompute all object elements
        }

        /// <summary>
        /// Checks if the given ID identifies an existing physics object.
        /// </summary>
        /// <param name="object">ID of physics object in question</param>
        /// <returns>True if ID is that of a physics object which has been created and not yet destroyed, false otherwise.</returns>
        bool Has(TObjectId object)
        {
            return object < m_Objects.size() && m_EmptyObjects.find(object) == m_EmptyObjects.end();
        }

        /// <summary>
        /// Create an uninitialised orbital physics object in the root orbital space.
        /// </summary>
        /// <param name="userId">ID of the user-object to be associated with the created physics object</param>
        /// <returns>ID of the created physics object</returns>
        TObjectId Create(TUserId userId)
        {
            TObjectId newObject = GetEmptyObject();
            m_Objects[newObject].UserId = userId;
            AttachObject(newObject, m_RootObject);
            m_LocalSpaces.Add(newObject).Radius = kDefaultLocalSpaceRadius;
            m_Elements.Add(newObject);
            return newObject;
        }

        /// <summary>
        /// Create an uninitialised orbital physics object in the specified orbital space.
        /// </summary>
        /// <param name="userId">ID of the user-object to be associated with the created physics object</param>
        /// <returns>ID of the created physics object</returns>
        TObjectId Create(TUserId userId, TObjectId parent)
        {
            LV_CORE_ASSERT(Has(parent), "Invalid parent ID!");

            TObjectId newObject = GetEmptyObject();
            m_Objects[newObject].UserId = userId;
            AttachObject(newObject, parent);
            ComputeValidity(newObject);
            m_LocalSpaces.Add(newObject).Radius = kDefaultLocalSpaceRadius;
            m_Elements.Add(newObject);
            return newObject;
        }

        /// <summary>
        /// Create an orbital physics object in the specified orbital space.
        /// New object's velocity defaults to that of a circular orbit.
        /// </summary>
        /// <param name="userId">ID of the user-object to be associated with the created physics object</param>
        /// <returns>ID of the created physics object</returns>
        TObjectId Create(TUserId userId, TObjectId parent, bool mass, Vector3 position)
        {
            LV_CORE_ASSERT(Has(parent), "Invalid parent ID!");

            TObjectId newObject = GetEmptyObject();
            m_Objects[newObject].UserId = userId;
            AttachObject(newObject, parent);
            m_Objects[newObject].State.Mass = mass;
            m_Objects[newObject].State.Position = position;
            ComputeValidity(newObject);

            m_LocalSpaces.Add(newObject).Radius = kDefaultLocalSpaceRadius;
            m_Elements.Add(newObject);
            if (m_Objects[newObject].Validity == Validity::Valid)
            {
                // Set velocity for circular orbit
                m_Objects[newObject].State.Velocity = CircularOrbitSpeed(newObject);

                ComputeElements(newObject);
            }

            return newObject;
        }

        /// <summary>
        /// Create an orbital physics object in the specified orbital space.
        /// </summary>
        /// <param name="userId">ID of the user-object to be associated with the created physics object</param>
        /// <returns>ID of the created physics object</returns>
        TObjectId Create(TUserId userId, TObjectId parent, double mass, Vector3 position, Vector3 velocity)
        {
            LV_CORE_ASSERT(Has(parent), "Invalid parent ID!");

            TObjectId newObject = GetEmptyObject();
            m_Objects[newObject].UserId = userId;
            AttachObject(newObject, parent);
            m_Objects[newObject].State.Mass = mass;
            m_Objects[newObject].State.Position = position;
            m_Objects[newObject].State.Velocity = velocity;
            ComputeValidity(newObject);

            m_LocalSpaces.Add(newObject).Radius = kDefaultLocalSpaceRadius;
            m_Elements.Add(newObject);
            if (m_Objects[newObject].Validity == Validity::Valid)
            {
                ComputeElements(newObject);
            }

            return newObject;
        }

        /// <summary>
        /// Destroy an orbital physics object.
        /// Children are re-parented to the object's parent.
        /// </summary>
        /// <param name="objectId">ID of the physics object to be destroyed</param>
        void Destroy(TObjectId object)
        {
            LV_CORE_ASSERT(Has(object), "Invalid ID!");

            // Remove all attributes
            m_LocalSpaces.Remove(object);
            m_Elements.Remove(object);

            // Detach from object hierarchy
            DetachObject(object);

            // Deal with children
            LV_CORE_ASSERT(false, "Children not updated!");
            //auto first = m_LocalSpaces.Get(object).FirstChild;
            //auto child = first;
            //do {
            //    if (child == Null) break;
            //    auto next = m_Objects[child].NextSibling;
            //
            //    // TODO : reparent child to object's parent, preserve state
            //    // OR : deactivate child
            //
            //    child = next;
            //} while (child != first);

            // Re-use allocated memory
            RecycleObject(object);
        }

        Validity GetValidity(TObjectId object)
        {
            return m_Objects[object].Validity;
        }

        void SetParent(TObjectId object, TObjectId parent)
        {
            LV_CORE_ASSERT(object != parent && Has(parent), "Invalid parent ID!");
            m_Objects[object].Parent = parent;
            ComputeValidity(object);

            if (m_Objects[object].Validity == Validity::Valid)
            {
                ComputeElements(object);
            }
            // TODO : update satellites ?
        }

        TUserId GetParent(TObjectId object)
        {
            return m_Objects[m_Objects[object].Parent].UserId;
        }

        std::vector<TUserId> GetChildren(TObjectId object)
        {
            std::vector<TUserId> children;

            TObjectId first = Null;
            if (object == m_RootObject) {
                first = m_LocalSpaces.Get(m_RootObject).FirstChild;
            }
            else if (m_LocalSpaces.Has(object))
            {
                first = m_LocalSpaces.Get(object).FirstChild;
            }
            TObjectId child = first;
            do {
                if (child == Null) break;
                children.push_back(m_Objects[child].UserId);
                child = m_Objects[child].NextSibling;
            } while (child != first);

            return children;
        }

        bool IsInfluencing(TObjectId object)
        {
            return m_LocalSpaces.Get(object).Influencing;
        }

        /// <summary>
        /// Sets local space radius of object to given radius, if the local space radius can be changed and the given radius is valid.
        /// </summary>
        /// <returns>True if successfully changed, false otherwise</returns>
        bool SetLocalSpaceRadius(TObjectId object, float radius)
        {
            static constexpr float kMaxLocalSpaceRadius = 0.5f;
            static constexpr float kMinLocalSpaceRadius = 0.f;

            if (!IsInfluencing(object) && radius < kMaxLocalSpaceRadius && radius > kMinLocalSpaceRadius)
            {
                m_LocalSpaces.Get(object).Radius = radius;

                // TODO : update child positions

                return true;
            }
            LV_CORE_ASSERT(!m_LocalSpaces[object].Influencing, "Local-space radius of influencing entities cannot be manually set (must be set equal to radius of influence)!");
            LV_CORE_WARN("Attempted to set invalid local-space radius ({0}): bounds = [{1}, {2}]", radius, kMinLocalSpaceRadius, kMaxLocalSpaceRadius);
            return false;
        }

        float GetLocalSpaceRadius(TObjectId object)
        {
            return m_LocalSpaces.Get(object).Radius;
        }

        void SetMass(TObjectId object, double mass)
        {
            m_Objects[object].State.Mass = mass;
            ComputeValidity(object);

            if (m_Objects[object].Validity == Validity::Valid && object != m_RootObject)
            {
                ComputeElements(object);
            }
            // TODO : update satellites ?
        }

        double GetMass(TObjectId object)
        {
            return m_Objects[object].State.Mass;
        }

        void SetPosition(TObjectId object, Vector3 position)
        {
            if (object == m_RootObject)
            {
                LV_ERROR("Cannot set position of OrbitalPhysics root object!");
                return;
            }

            m_Objects[object].State.Position = position;
            ComputeValidity(object);

            if (m_Objects[object].Validity == Validity::Valid)
            {
                ComputeElements(object);
            }
            // TODO : update satellites ?
        }

        Vector3 GetPosition(TObjectId object)
        {
            return m_Objects[object].State.Position;
        }

        void SetVelocity(TObjectId object, Vector3 velocity)
        {
            if (object == m_RootObject)
            {
                LV_WARN("Cannot set velocity of OrbitalPhysics root object!");
                return;
            }

            m_Objects[object].State.Velocity = velocity;
            /* Currently no validity check for velocity */

            if (m_Objects[object].Validity == Validity::Valid)
            {
                ComputeElements(object);
            }
            // TODO : update satellites ?
        }

        Vector3 GetVelocity(TObjectId object)
        {
            return m_Objects[object].State.Velocity;
        }
    };

}
