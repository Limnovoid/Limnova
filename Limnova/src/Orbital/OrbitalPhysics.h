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
        static constexpr Vector3 kReferenceX = { 1.f, 0.f, 0.f };
        static constexpr Vector3 kReferenceY = { 0.f, 1.f, 0.f };
        static constexpr Vector3 kReferenceNormal = { 0.f, 0.f, 1.f };

        static constexpr double kGravitational = 6.6743e-11;
        static constexpr float kDefaultLocalSpaceRadius = 0.1f;
        static constexpr float kParallelDotProductLimit = 1.f - 1e-4f;
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
            Attr& operator[](TObjectId object)
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
            Vector3d Velocity = { 0.0, 0.0, 0.0 };
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
    public:
        struct Elements
        {
            double Grav = 0.f;          /* Gravitational parameter (mu) */
            double H = 0.0;             /* Orbital specific angular momentum */
            double E = { 0.f };         /* Eccentricity */

            float I = 0.f;              /* Inclination */
            Vector3 N = { 0.f };        /* Direction of ascending node */
            float Omega = 0.f;          /* Right ascension of ascending node */
            float ArgPeriapsis = 0.f;   /* Argument of periapsis */

            /* Basis of the perifocal frame */
            Vector3 PerifocalX = { 0.f }, PerifocalY = { 0.f }, PerifocalNormal = { 0.f };
            Quaternion PerifocalOrientation;

            float TrueAnomaly = 0.f;

            float SemiMajor = 0.f, SemiMinor = 0.f;
            float C = 0.f; /* Signed distance from occupied focus to centre, measured in perifocal frame's x-axis */
            double T = 0.0;
        };
    private:
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
            static constexpr float kEscapeDistanceSqr = 1.01f * 1.01f;

            return m_Objects[object].State.Position.SqrMagnitude() < kEscapeDistanceSqr;
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
            if (m_LocalSpaces[m_RootObject].MetersPerRadius > 0.0) {
                return m_LocalSpaces.Has(m_Objects[object].Parent) || m_Objects[object].Parent == m_RootObject || object == m_RootObject;
            }
            LV_WARN("OrbitalPhysics root scaling has not been set!");
            return false;
        }

        /// <summary>
        /// Currently ignores velocity - no invalid velocities.
        /// </summary>
        /// <returns>True if object state is valid, false otherwise</returns>
        bool ComputeStateValidity(TObjectId object)
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
            return validity == Validity::Valid;
        }

        void ComputeInfluence(TObjectId object)
        {
            /* Radius of influence = a(m / M)^0.4
             * Semi-major axis must be in the order of 1,
             * so the order of ROI is determined by (m / M)^0.4 */
            static constexpr double kMinimumMassFactor = 1e-4;
            double massFactor = pow(m_Objects[object].State.Mass / m_Objects[m_Objects[object].Parent].State.Mass, 0.4);
            if (massFactor > kMinimumMassFactor)
            {
                m_LocalSpaces[object].Influencing = true;
                m_LocalSpaces[object].Radius = m_Elements[object].SemiMajor * massFactor;
            }
            /* If already non-influencing, local-space radius may have been explicitly set by user:
             * only reset it to default radius if this is not the case */
            else if (m_LocalSpaces[object].Influencing)
            {
                m_LocalSpaces[object].Influencing = false;
                m_LocalSpaces[object].Radius = kDefaultLocalSpaceRadius;
            }
        }

        void ComputeElements(TObjectId object)
        {
            LV_CORE_ASSERT(object != m_RootObject, "Cannot compute elements on root object!")

            auto& obj = m_Objects[object];
            auto& state = obj.State;
            auto& elems = m_Elements.Get(object);

            LV_CORE_ASSERT(obj.Validity == Validity::Valid, "Cannot compute elements on an invalid object!");

            elems.Grav = kGravitational * m_Objects[obj.Parent].State.Mass * pow(m_LocalSpaces.Get(obj.Parent).MetersPerRadius, -3.0);

            Vector3d Hvec = Vector3d(state.Position).Cross(state.Velocity);
            double H2 = Hvec.SqrMagnitude();
            elems.H = sqrt(H2);
            elems.PerifocalNormal = Hvec / elems.H;

            /* Loss of precision due to casting is acceptable: result of vector division (V x H / Grav) is on the order of 1 */
            Vector3 posDir = state.Position.Normalized();
            Vector3 Evec = (Vector3)(state.Velocity.Cross(Hvec) / elems.Grav) - posDir;
            elems.E = sqrtf(Evec.SqrMagnitude());
            static constexpr float kEccentricityEpsilon = 1e-4f;
            if (elems.E < kEccentricityEpsilon)
            {
                // Circular
                elems.E = 0.f;
                elems.PerifocalX = abs(elems.PerifocalNormal.Dot(kReferenceY)) > kParallelDotProductLimit
                    ? kReferenceX : kReferenceY.Cross(elems.PerifocalNormal);
                elems.PerifocalY = elems.PerifocalNormal.Cross(elems.PerifocalX);
            }
            else
            {
                // Elliptical
                elems.PerifocalX = Evec / elems.E;
                elems.PerifocalY = elems.PerifocalNormal.Cross(elems.PerifocalX);
            }

            elems.I = acosf(elems.PerifocalNormal.Dot(kReferenceNormal));
            elems.N = abs(elems.PerifocalNormal.Dot(kReferenceNormal)) > kParallelDotProductLimit
                ? kReferenceX : kReferenceNormal.Cross(elems.PerifocalNormal).Normalized();
            elems.Omega = acosf(elems.N.Dot(kReferenceX));
            if (elems.N.Dot(kReferenceY) < 0.f) {
                elems.Omega = PI2f - elems.Omega;
            }
            elems.ArgPeriapsis = acosf(elems.N.Dot(elems.PerifocalX));
            if (elems.N.Dot(elems.PerifocalY) > 0.f) {
                elems.ArgPeriapsis = PI2f - elems.ArgPeriapsis;
            }
            //elems.PerifocalOrientation =
            //    Quaternion(kReferenceNormal, elems.Omega)
            //    * Quaternion(elems.PerifocalX, elems.I)
            //    * Quaternion(elems.PerifocalNormal, elems.ArgPeriapsis);
            elems.PerifocalOrientation =
                Quaternion(elems.PerifocalNormal, elems.ArgPeriapsis);

            elems.TrueAnomaly = acosf(elems.PerifocalX.Dot(posDir));
            // Disambiguate based on whether the position is in the positive or negative Y-axis of the perifocal frame
            if (posDir.Dot(elems.PerifocalY) < 0.f)
            {
                // Velocity is in the negative X-axis of the perifocal frame
                elems.TrueAnomaly = PI2f - elems.TrueAnomaly;
            }

            // Dimensions
            float e2 = powf(elems.E, 2.f);
            /* Loss of precision due to casting is acceptable: semi-major axis is on the order of 1 in all common cases, due to distance parameterisation */
            float H2_Grav = (float)(H2 / elems.Grav);
            elems.SemiMajor = H2_Grav / (1.f - e2);
            elems.SemiMinor = elems.SemiMajor * sqrtf(1.f - e2);

            elems.T = pow((double)elems.SemiMajor, 1.5) * PI2 / sqrt(elems.H);
            elems.C = H2_Grav / (1.f + elems.E) - elems.SemiMajor;
        }

        /// <summary>
        /// Returns speed for a circular orbit around the given primary at the given distance.
        /// Assumes orbiter has insignificant mass compared to primary.
        /// </summary>
        /// <param name="object">Physics object ID</param>
        double CircularOrbitSpeed(TObjectId primary, float radius)
        {
            LV_CORE_ASSERT(m_LocalSpaces[primary].Influencing, "Cannot request circular orbit speed around an object which cannot be orbited!");

            /* ||V_circular|| = sqrt(mu / ||r||), where mu is the gravitational parameter of the orbit */
            return sqrt(kGravitational * m_Objects[primary].State.Mass * pow(m_LocalSpaces[primary].MetersPerRadius, -3.0) / (double)radius);
        }

        /// <summary>
        /// Returns velocity for a circular counter-clockwise orbit around the given primary at the given position.
        /// Assumes orbiter has insignificant mass compared to primary.
        /// </summary>
        /// <param name="object">Physics object ID</param>
        Vector3d CircularOrbitVelocity(TObjectId primary, const Vector3& position)
        {
            /* Keep the orbital plane as flat (close to the reference plane) as possible:
             * derive velocity direction as the cross product of reference normal and normalized position */
            Vector3d vDir;
            float rMag = sqrtf(position.SqrMagnitude());
            Vector3 rDir = position / rMag;

            static constexpr float kParallelDotProductLimit = 1.f - 1e-4f;
            float rDotNormal = rDir.Dot(kReferenceNormal);
            if (abs(rDotNormal) > kParallelDotProductLimit)
            {
                /* Handle cases where the normal and position are parallel:
                 * counter-clockwise around the reference Y-axis, whether above or below the plane */
                vDir = rDotNormal > 0.f ? (Vector3d)(-kReferenceX) : (Vector3d)kReferenceX;
            }
            else
            {
                vDir = (Vector3d)(kReferenceNormal.Cross(rDir).Normalized());
            }
            return vDir * CircularOrbitSpeed(primary, rMag);
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

        double GetRootScaling()
        {
            return m_LocalSpaces[m_RootObject].MetersPerRadius;
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

            m_LocalSpaces.Add(newObject).Radius = kDefaultLocalSpaceRadius;
            m_Elements.Add(newObject);

            if (ComputeStateValidity(newObject))
            {
                ComputeElements(newObject);
                ComputeInfluence(newObject);
            }
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

            //TObjectId newObject = GetEmptyObject();
            //m_Objects[newObject].UserId = userId;
            //AttachObject(newObject, parent);
            //m_Objects[newObject].State.Mass = mass;
            //m_Objects[newObject].State.Position = position;
            //
            //m_LocalSpaces.Add(newObject).Radius = kDefaultLocalSpaceRadius;
            //m_Elements.Add(newObject);
            //if (ComputeStateValidity(newObject))
            //{
            //    m_Objects[newObject].State.Velocity = GetDefaultOrbitVelocity(newObject);
            //
            //    ComputeElements(newObject);
            //    ComputeInfluence(newObject);
            //}

            return Create(userId, parent, mass, position, CircularOrbitVelocity(parent, position));
        }

        /// <summary>
        /// Create an uninitialised orbital physics object in the specified orbital space.
        /// </summary>
        /// <param name="userId">ID of the user-object to be associated with the created physics object</param>
        /// <returns>ID of the created physics object</returns>
        TObjectId Create(TUserId userId, TObjectId parent)
        {
            LV_CORE_ASSERT(Has(parent), "Invalid parent ID!");

            //TObjectId newObject = GetEmptyObject();
            //m_Objects[newObject].UserId = userId;
            //AttachObject(newObject, parent);
            //
            //ComputeStateValidity(newObject);
            //
            //m_LocalSpaces.Add(newObject).Radius = kDefaultLocalSpaceRadius;
            //m_Elements.Add(newObject);
            //return newObject;

            return Create(userId, parent, 0.0, { 0.f }, { 0.0 });
        }

        /// <summary>
        /// Create an uninitialised orbital physics object in the root orbital space.
        /// </summary>
        /// <param name="userId">ID of the user-object to be associated with the created physics object</param>
        /// <returns>ID of the created physics object</returns>
        TObjectId Create(TUserId userId)
        {
            //TObjectId newObject = GetEmptyObject();
            //m_Objects[newObject].UserId = userId;
            //AttachObject(newObject, m_RootObject);
            //m_LocalSpaces.Add(newObject).Radius = kDefaultLocalSpaceRadius;
            //m_Elements.Add(newObject);
            //return newObject;

            return Create(userId, m_RootObject, 0.0, { 0.f }, { 0.0 });
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
            ComputeStateValidity(object);

            if (m_Objects[object].Validity == Validity::Valid)
            {
                ComputeElements(object);
                ComputeInfluence(object);
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
            static constexpr float kMaxLocalSpaceRadius = 0.25f;
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
            ComputeStateValidity(object);

            if (m_Objects[object].Validity == Validity::Valid && object != m_RootObject)
            {
                ComputeElements(object);
                ComputeInfluence(object);
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
            ComputeStateValidity(object);

            if (m_Objects[object].Validity == Validity::Valid)
            {
                ComputeElements(object);
                ComputeInfluence(object);
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
                ComputeInfluence(object);
            }
            // TODO : update satellites ?
        }

        Vector3d GetVelocity(TObjectId object)
        {
            return m_Objects[object].State.Velocity;
        }

        /// <summary>
        /// Returns velocity for a circular counter-clockwise orbit around the object's current primary, given its current mass and position.
        /// </summary>
        /// <param name="object">Physics object ID</param>
        Vector3d GetDefaultOrbitVelocity(TObjectId object)
        {
            /* Keep the orbital plane as flat (close to the reference plane) as possible:
             * derive velocity direction as the cross product of reference normal and normalized position */
            Vector3d vDir;
            float rMag = sqrtf(m_Objects[object].State.Position.SqrMagnitude());
            Vector3 rDir = m_Objects[object].State.Position / rMag;

            float rDotNormal = rDir.Dot(kReferenceNormal);
            if (abs(rDotNormal) > kParallelDotProductLimit)
            {
                /* Handle cases where the normal and position are parallel:
                 * counter-clockwise around the reference Y-axis, whether above or below the plane */
                vDir = rDotNormal > 0.f ? -kReferenceX : kReferenceX;
            }
            else
            {
                vDir = (Vector3d)kReferenceNormal.Cross(rDir).Normalized();
            }
            return vDir * CircularOrbitSpeed(m_Objects[object].Parent, rMag);
        }

        const Elements& GetElements(TObjectId object)
        {
            return m_Elements[object];
        }
    };

}
