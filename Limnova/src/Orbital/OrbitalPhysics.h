#pragma once

#include <Math/Math.h>
#include <Core/Timestep.h>


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
        /* Basis of the reference frame: the XY-plane represents the orbital plane of the system which has the root object as its primary */
        static constexpr Vector3 kReferenceX        = { 1.f, 0.f, 0.f };
        static constexpr Vector3 kReferenceY        = { 0.f, 0.f,-1.f };
        static constexpr Vector3 kReferenceNormal   = { 0.f, 1.f, 0.f };

        static constexpr double kGravitational = 6.6743e-11;
        static constexpr float kDefaultLocalSpaceRadius = 0.1f;
        static constexpr float kLocalSpaceEscapeRadius = 1.01f;
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
            InvalidPath     = 3,
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
            float E = { 0.f };          /* Eccentricity */

            float P = 0.f;              /* Orbit parameter, or semi-latus rectum */

            float I = 0.f;              /* Inclination */
            Vector3 N = { 0.f };        /* Direction of ascending node */
            float Omega = 0.f;          /* Right ascension of ascending node */
            float ArgPeriapsis = 0.f;   /* Argument of periapsis */

            /* Basis of the perifocal frame */
            Vector3 PerifocalX = { 0.f }, PerifocalY = { 0.f }, PerifocalNormal = { 0.f };
            Quaternion PerifocalOrientation; /* Orientation of the perifocal frame relative to the reference frame */

            float TrueAnomaly = 0.f;

            float SemiMajor = 0.f, SemiMinor = 0.f; /* Semi-major and semi-minor axes */
            float C = 0.f;              /* Signed distance from occupied focus to centre, measured along perifocal frame's x-axis */
            double T = 0.0;             /* Orbit period, measured in seconds */
        };

        struct Dynamics
        {
            float EscapeTrueAnomaly = 0.f;  /* True anomaly at which orbital radius equals the local-space escape radius */
            Vector3 EscapePoint = { 0.f }; /* Point on the orbit at which the orbiter will cross the primary's local space boundary to exit the local space */
            Vector3 EntryPoint = { 0.f }; /* Point on the orbit at which the orbiter (would have) crossed the primary's local space boundary to enter the local space */
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
        AttributeStorage<Dynamics> m_Dynamics;
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
            static constexpr float kEscapeDistanceSqr = kLocalSpaceEscapeRadius * kLocalSpaceEscapeRadius;

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
            /* Currently ignores velocity - no invalid velocities */

            m_Objects[object].Validity = validity;
            return validity == Validity::Valid;
        }

        inline float OrbitEquation(TObjectId object, float trueAnomaly)
        {
            return m_Elements[object].P / (1.f + m_Elements[object].E * cosf(trueAnomaly));
        }

        Vector3 ObjectPositionAtTrueAnomaly(TObjectId object, float trueAnomaly)
        {
            float radiusAtTrueAnomaly = OrbitEquation(object, trueAnomaly);
            Vector3 directionAtTrueAnomaly = cosf(trueAnomaly) * m_Elements[object].PerifocalX
                + sinf(trueAnomaly) * m_Elements[object].PerifocalY;
            return radiusAtTrueAnomaly * directionAtTrueAnomaly;
        }

        void ComputeInfluence(TObjectId object)
        {
            LV_CORE_ASSERT(object != m_RootObject, "Cannot compute influence of root object!");

            auto& ls = m_LocalSpaces[object];
            TObjectId parent = m_Objects[object].Parent;

            /* Radius of influence = a(m / M)^0.4
             * Semi-major axis must be in the order of 1,
             * so the order of ROI is determined by (m / M)^0.4 */
            static constexpr double kMinimumMassFactor = 1e-4;
            double massFactor = pow(m_Objects[object].State.Mass / m_Objects[parent].State.Mass, 0.4);
            if (massFactor > kMinimumMassFactor)
            {
                ls.Influencing = true;
                ls.Radius = m_Elements[object].SemiMajor * massFactor;
                ls.MetersPerRadius = m_LocalSpaces[parent].MetersPerRadius * ls.Radius;
            }
            /* If already non-influencing, local-space radius may have been set by the user:
             * only reset it to default radius if this is not the case */
            else if (ls.Influencing)
            {
                ls.Influencing = false;
                ls.Radius = kDefaultLocalSpaceRadius;
                ls.MetersPerRadius = m_LocalSpaces[parent].MetersPerRadius * ls.Radius;
            }
        }

        void ComputeDynamics(TObjectId object)
        {
            LV_CORE_ASSERT(object != m_RootObject, "Cannot compute dynamics on root object!");

            auto& obj = m_Objects[object];
            auto& elems = m_Elements[object];

            float apoapsisRadius = elems.P / (1.f - elems.E);
            bool escapesLocalSpace = apoapsisRadius > kLocalSpaceEscapeRadius;

            float escapeTrueAnomaly = 0.f;
            if (escapesLocalSpace) {
                escapeTrueAnomaly = acosf((elems.P / kLocalSpaceEscapeRadius - 1.f) / elems.E);
            }

            // TODO : #ifdef to exclude Validity from release ?
            LV_CORE_ASSERT(obj.Validity == Validity::Valid || obj.Validity == Validity::InvalidPath,
                "Cannot compute dynamics on object with invalid parent, mass, or position!");

            obj.Validity = Validity::Valid;
            if (m_Dynamics.Has(object))
            {
                if (escapesLocalSpace && obj.Parent == m_RootObject)
                {
                    LV_WARN("Orbit path cannot exit the simulation space!");
                    obj.Validity = Validity::InvalidPath;
                    return;
                }
            }
            else
            {
                bool invalidPath = false;
                if (escapesLocalSpace) {
                    LV_WARN("Non-dynamic orbit cannot exit its primary's local space!");
                    invalidPath = true;
                }
                /* TODO : test for other dynamic orbit events */
                if (invalidPath) {
                    obj.Validity = Validity::InvalidPath;
                }
                return;
            }

            auto& dynamics = m_Dynamics.Get(object);

            dynamics.EscapeTrueAnomaly = escapeTrueAnomaly;
            dynamics.EscapePoint = escapesLocalSpace ? ObjectPositionAtTrueAnomaly(object, escapeTrueAnomaly) : Vector3{ 0.f };
            dynamics.EntryPoint = escapesLocalSpace ? ObjectPositionAtTrueAnomaly(object, PI2f - escapeTrueAnomaly) : Vector3{ 0.f };
        }

        void ComputeElements(TObjectId object)
        {
            LV_CORE_ASSERT(object != m_RootObject, "Cannot compute elements on root object!");

            auto& obj = m_Objects[object];
            auto& state = obj.State;
            auto& elems = m_Elements.Get(object);

            LV_CORE_ASSERT(obj.Validity == Validity::Valid || obj.Validity == Validity::InvalidPath,
                "Cannot compute elements on an object with invalid parent, mass, or position!");

            elems.Grav = kGravitational * m_Objects[obj.Parent].State.Mass * pow(m_LocalSpaces.Get(obj.Parent).MetersPerRadius, -3.0);

            Vector3d Hvec = Vector3d(state.Position).Cross(state.Velocity);
            double H2 = Hvec.SqrMagnitude();
            elems.H = sqrt(H2);
            elems.PerifocalNormal = Hvec / elems.H;

            /* Loss of precision due to casting is acceptable: semi-latus recturm is on the order of 1 in all common cases, due to distance parameterisation */
            elems.P = (float)(H2 / elems.Grav);

            /* Loss of precision due to casting is acceptable: result of vector division (V x H / Grav) is on the order of 1 */
            Vector3 posDir = state.Position.Normalized();
            Vector3 Evec = (Vector3)(state.Velocity.Cross(Hvec) / elems.Grav) - posDir;
            elems.E = sqrtf(Evec.SqrMagnitude());
            float e2 = powf(elems.E, 2.f), e2term;
            static constexpr float kEccentricityEpsilon = 1e-4f;
            if (elems.E < kEccentricityEpsilon)
            {
                // Circular
                elems.E = 0.f;
                elems.PerifocalX = abs(elems.PerifocalNormal.Dot(kReferenceY)) > kParallelDotProductLimit
                    ? kReferenceX : kReferenceY.Cross(elems.PerifocalNormal);
                elems.PerifocalY = elems.PerifocalNormal.Cross(elems.PerifocalX);

                e2term = 1.f;
            }
            else
            {
                elems.PerifocalX = Evec / elems.E;
                elems.PerifocalY = elems.PerifocalNormal.Cross(elems.PerifocalX);

                if (elems.E < 1.f) {
                    // Elliptical
                    e2term = 1.f - e2;
                }
                else {
                    // Hyperbolic
                    e2term = 1.f + e2;
                }
            }

            // Dimensions
            elems.SemiMajor = elems.P / (e2term);
            elems.SemiMinor = elems.SemiMajor * sqrtf(e2term);

            elems.C = elems.P / (1.f + elems.E);
            elems.C += elems.E < 1.f ? -elems.SemiMajor : elems.SemiMajor; /* differentiate center position for ellipse and hyperbola */

            elems.T = pow((double)elems.SemiMajor, 1.5) * PI2 / sqrt(elems.H);

            elems.TrueAnomaly = AngleBetweenUnitVectorsSafe(elems.PerifocalX, posDir);
            // Disambiguate based on whether the position is in the positive or negative Y-axis of the perifocal frame
            if (posDir.Dot(elems.PerifocalY) < 0.f)
            {
                // Velocity is in the negative X-axis of the perifocal frame
                elems.TrueAnomaly = PI2f - elems.TrueAnomaly;
            }

            // Frame orientation
            elems.I = acosf(elems.PerifocalNormal.Dot(kReferenceNormal));
            elems.N = abs(elems.PerifocalNormal.Dot(kReferenceNormal)) > kParallelDotProductLimit
                ? elems.PerifocalX : kReferenceNormal.Cross(elems.PerifocalNormal).Normalized();
            elems.Omega = acosf(elems.N.Dot(kReferenceX));
            if (elems.N.Dot(kReferenceY) < 0.f) {
                elems.Omega = PI2f - elems.Omega;
            }
            elems.ArgPeriapsis = AngleBetweenUnitVectorsSafe(elems.N, elems.PerifocalX);
            if (elems.N.Dot(elems.PerifocalY) > 0.f) {
                elems.ArgPeriapsis = PI2f - elems.ArgPeriapsis;
            }
            elems.PerifocalOrientation =
                Quaternion(elems.PerifocalNormal, elems.ArgPeriapsis)
                * Quaternion(elems.N, elems.I)
                * Quaternion(kReferenceNormal, elems.Omega);
        }

        void TryComputeOrbitalState(TObjectId object)
        {
            if (object != m_RootObject && (m_Objects[object].Validity == Validity::Valid ||
                m_Objects[object].Validity == Validity::InvalidPath))
            {
                ComputeElements(object);
                ComputeDynamics(object); /* If orbiter is not dynamic, sets Validity to InvalidPath if dynamic events are found */
                ComputeInfluence(object);
            }
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

        void OnUpdate(Timestep dT)
        {
        }


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
        TObjectId Create(TUserId userId, TObjectId parent, double mass, Vector3 position, Vector3 velocity, bool dynamic = false)
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

            if (dynamic) {
                m_Dynamics.Add(newObject);
            }

            ComputeStateValidity(newObject);
            TryComputeOrbitalState(newObject);

            return newObject;
        }

        /// <summary>
        /// Create an orbital physics object in the specified orbital space.
        /// New object's velocity defaults to that of a circular orbit.
        /// </summary>
        /// <param name="userId">ID of the user-object to be associated with the created physics object</param>
        /// <returns>ID of the created physics object</returns>
        TObjectId Create(TUserId userId, TObjectId parent, bool mass, Vector3 position, bool dynamic = false)
        {
            LV_CORE_ASSERT(Has(parent), "Invalid parent ID!");

            return Create(userId, parent, mass, position, CircularOrbitVelocity(parent, position), dynamic);
        }

        /// <summary>
        /// Create an uninitialised orbital physics object in the specified orbital space.
        /// </summary>
        /// <param name="userId">ID of the user-object to be associated with the created physics object</param>
        /// <returns>ID of the created physics object</returns>
        TObjectId Create(TUserId userId, TObjectId parent, bool dynamic = false)
        {
            LV_CORE_ASSERT(Has(parent), "Invalid parent ID!");

            return Create(userId, parent, 0.0, { 0.f }, { 0.0 }, dynamic);
        }

        /// <summary>
        /// Create an uninitialised orbital physics object in the root orbital space.
        /// </summary>
        /// <param name="userId">ID of the user-object to be associated with the created physics object</param>
        /// <returns>ID of the created physics object</returns>
        TObjectId Create(TUserId userId, bool dynamic = false)
        {
            return Create(userId, m_RootObject, 0.0, { 0.f }, { 0.0 }, dynamic);
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
            TryComputeOrbitalState(object);
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
            static constexpr float kMinLocalSpaceRadius = 0.01f;

            if (!IsInfluencing(object) && radius < kMaxLocalSpaceRadius && radius > kMinLocalSpaceRadius)
            {
                m_LocalSpaces.Get(object).Radius = radius;

                // TODO : update child positions

                return true;
            }
            LV_CORE_ASSERT(!m_LocalSpaces[object].Influencing, "Local-space radius of influencing entities cannot be manually set (must be set equal to radius of influence)!");
            LV_CORE_WARN("Attempted to set invalid local-space radius ({0}): must be in the range [{1}, {2}]", radius, kMinLocalSpaceRadius, kMaxLocalSpaceRadius);
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
            TryComputeOrbitalState(object); /* NOTE: this should be redundant as orbital motion is independent of orbiter mass, but do it anyway just for consistency */
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
            TryComputeOrbitalState(object);
            // TODO : update satellites ?
        }

        Vector3 GetPosition(TObjectId object)
        {
            return m_Objects[object].State.Position;
        }


        void SetVelocity(TObjectId object, Vector3d velocity)
        {
            if (object == m_RootObject)
            {
                LV_WARN("Cannot set velocity of OrbitalPhysics root object!");
                return;
            }

            m_Objects[object].State.Velocity = velocity;
            TryComputeOrbitalState(object);
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


        void SetDynamic(TObjectId object, bool isDynamic)
        {
            if (isDynamic)
            {
                m_Dynamics.GetOrAdd(object);
            }
            else
            {
                m_Dynamics.TryRemove(object);
            }

            TryComputeOrbitalState(object);
        }

        bool IsDynamic(TObjectId object)
        {
            return m_Dynamics.Has(object);
        }

        const Dynamics& GetDynamics(TObjectId object)
        {
            return m_Dynamics[object];
        }

    };

}
