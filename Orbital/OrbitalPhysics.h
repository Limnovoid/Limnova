#pragma once

#include "Limnova.h"


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
        ~OrbitalPhysics()
        {
            // Estimating maximum object allocation for optimising vectors -> arrays
            LV_CORE_INFO("OrbitalPhysics final object count: {0}", m_Objects.size());
        }

        using TObjectId = uint32_t;
        static constexpr TObjectId Null = std::numeric_limits<TObjectId>::max();
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
            double Mass = 0.0;
            Vector3 Position = { 0.f, 0.f, 0.f };
            Vector3 Velocity = { 0.f, 0.f, 0.f };
        };

        struct Object
        {
            TUserId UserId = TUserId();
            TObjectId Parent = Null;
            Validity Validity = Validity::InvalidParent;
            State State;

            Object() = default;
            Object(TUserId userId) : UserId(userId) {}
        };


        /*** Attributes ***/

        struct Elements
        {
            Vector3 H = { 0.f, 0.f, 0.f };
        };

        struct Children
        {
            std::vector<TObjectId> Children;
        };

        struct Influence
        {
            float Radius = 0.f;
        };
    private:
        /*** Simulation resources ***/

        Object m_RootObject;
        double m_RootMass;
        float m_RootScalingRadius; // orbital radii per metre in root scaling space
        Children m_RootChildren;

        std::vector<Object> m_Objects;
        std::unordered_set<TObjectId> m_EmptyObjects;

        AttributeStorage<Elements> m_Elements;
        AttributeStorage<Children> m_Children;
        AttributeStorage<Influence> m_Influences;
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
            m_Objects[object].Parent = Null;
            m_Objects[object].Validity = Validity::InvalidParent;
            m_EmptyObjects.insert(object);
        }


        /*** Simulation helpers ***/

        bool ValidPosition(TObjectId object)
        {
            return m_Objects[object].State.Position.SqrMagnitude() < 1.f
                /* TODO - check for influence overlaps */;
        }

        bool ValidMass(TObjectId object)
        {
            static constexpr double kMaxCOG = 1e-6; /* Maximum offset for shared centre of gravity */
            return kMaxCOG > m_Objects[object].State.Mass / (m_Objects[object].State.Mass + m_Objects[m_Objects[object].Parent].State.Mass);
        }

        bool ValidParent(TObjectId object)
        {
            return m_Influences.Has(m_Objects[object].Parent);
        }

        void ComputeValidity(TObjectId object)
        {
            // TODO : #ifdef LV_DEBUG ??? is validity needed in release?

            Validity validity = Validity::Valid;
            if (!ValidParent(object))
            {
                validity = Validity::InvalidParent;
            }
            else if (!ValidMass(object))
            {
                validity = Validity::InvalidMass;
            }
            else if (!ValidPosition(object))
            {
                validity = Validity::InvalidPosition;
            }
            m_Objects[object].Validity = validity;
        }

        void ComputeElements(TObjectId object)
        {
            auto& obj = m_Objects[object];
            auto& state = obj.State;
            auto& elems = m_Elements.Get(object);

            LV_CORE_ASSERT(obj.Validity == Validity::Valid, "Cannot compute elements on an invalid object!");

            // TODO
        }
    public:
        /*** Usage ***/

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
            m_Objects[newObject].Parent = parent;
            ComputeValidity(newObject);
            return newObject;
        }

        /// <summary>
        /// Create an orbital physics object in the specified orbital space.
        /// New object's velocity defaults to that of a circular orbit.
        /// </summary>
        /// <param name="userId">ID of the user-object to be associated with the created physics object</param>
        /// <returns>ID of the created physics object</returns>
        TObjectId Create(TUserId userId, TObjectId parent, Vector3 position)
        {
            LV_CORE_ASSERT(Has(parent), "Invalid parent ID!");

            TObjectId newObject = GetEmptyObject();
            m_Objects[newObject].UserId = userId;
            m_Objects[newObject].Parent = parent;
            m_Objects[newObject].State.Position = position;
            ComputeValidity(newObject);
            if (m_Objects[newObject].Validity == Validity::Valid)
            {
                // TODO - default velocity to circular
                ComputeElements(newObject);
            }
            return newObject;
        }

        /// <summary>
        /// Create an orbital physics object in the specified orbital space.
        /// </summary>
        /// <param name="userId">ID of the user-object to be associated with the created physics object</param>
        /// <returns>ID of the created physics object</returns>
        TObjectId Create(TUserId userId, TObjectId parent, Vector3 position, Vector3 velocity)
        {
            LV_CORE_ASSERT(Has(parent), "Invalid parent ID!");

            TObjectId newObject = GetEmptyObject();
            m_Objects[newObject].UserId = userId;
            m_Objects[newObject].Parent = parent;
            auto& state = m_Objects[newObject].State.Add(newObject);
            state.Position = position;
            state.Velocity = velocity;
            ComputeValidity(newObject);
            if (m_Objects[newObject].Validity == Validity::Valid)
            {
                ComputeElements(newObject);
            }
            return newObject;
        }

        /// <summary>
        /// Destroy an orbital physics object.
        /// </summary>
        /// <param name="objectId">ID of the physics object to be destroyed</param>
        void Destroy(TObjectId object)
        {
            LV_CORE_ASSERT(Has(object), "Invalid ID!");

            // Remove all attributes
            m_Influences.TryRemove(object);

            // TODO : destroy satellites ???

            RecycleObject(object);
        }

        void SetParent(TObjectId object, TObjectId parent)
        {
            LV_CORE_ASSERT(object != parent && Has(object) && Has(parent), "Invalid IDs!");
            m_Objects[object].Parent = parent;
            ComputeValidity(object);

            if (m_Objects[object].Validity == Validity::Valid)
            {
                ComputeElements(object);
            }
            // TODO : update satellites ?
        }

        void SetMass(TObjectId object, double mass)
        {
            LV_CORE_ASSERT(Has(object), "Invalid ID!");

            m_Objects[object].State.Mass = mass;
            ComputeValidity(object);

            if (m_Objects[object].Validity == Validity::Valid)
            {
                ComputeElements(object);
            }
            // TODO : update satellites ?
        }

        void SetPosition(TObjectId object, Vector3 position)
        {
            LV_CORE_ASSERT(Has(object), "Invalid ID!");

            m_Objects[object].State.Position = position;
            ComputeValidity(object);

            if (m_Objects[object].Validity == Validity::Valid)
            {
                ComputeElements(object);
            }
            // TODO : update satellites ?
        }

        void SetVelocity(TObjectId object, Vector3 velocity)
        {
            LV_CORE_ASSERT(Has(object), "Invalid ID!");

            m_Objects[object].State.Velocity = velocity;
            /* Currently no validity check for velocity */

            if (m_Objects[object].Validity == Validity::Valid)
            {
                ComputeElements(object);
            }
            // TODO : update satellites ?
        }
    };

}
