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
            m_Tree.New(); /* kRootObjId (0) */
            m_Tree.New(); /* kRootLspId (1) */

            auto& rootObj = m_Objects.Add(kRootObjId);
            rootObj.Validity = Validity::InvalidMass; /* Object::Validity is initialised to InvalidParent but that is meaningless for the root object (which cannot be parented) */
            
            auto& rootLsp = m_LSpaces.Add(kRootLspId);
            rootLsp.Radius = 1.f;
            rootLsp.Influencing = true;
        }
        OrbitalPhysics(const OrbitalPhysics&) = default;
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


        // Simulation tuning parameters ////////
        // TODO : choose numbers based on reasoning/testing
        static constexpr float kDefaultLocalSpaceRadius = 0.1f;
        static constexpr float kLocalSpaceEscapeRadius = 1.01f;

        static constexpr float kEccentricityEpsilon = 1e-4f;

        static constexpr float kMaxLocalSpaceRadius = 0.2f;
        static constexpr float kMinLocalSpaceRadius = 0.01f;
        static constexpr float kEpsLocalSpaceRadius = 1e-6f;

        static constexpr float kMaxObjectUpdates = 20.f;
        static constexpr double kDefaultMinDT = 1.0 / (60.0 * kMaxObjectUpdates);
        static constexpr float kMinUpdateDistance = 1e-5f;
        static constexpr double kMinUpdateDistanced = (double)kMinUpdateDistance;
        static constexpr float kMinUpdateTrueAnomaly = 1e-5f;
        ////////////////////////////////////////


        /*** Object tree ***/
    private:
        using TNodeId = uint32_t;

        static constexpr TNodeId NNull = std::numeric_limits<TNodeId>::max();

        struct Node
        {
            TNodeId Parent = NNull;
            TNodeId NextSibling = NNull;
            TNodeId PrevSibling = NNull;
            TNodeId FirstChild = NNull;
        };

        class Tree
        {
            std::vector<Node> m_Nodes;
            std::vector<int> m_Heights;
            std::unordered_set<TNodeId> m_Empties;
        public:
            Tree() = default;
            Tree(const Tree&) = default;

            bool Has(TNodeId nodeId)
            {
                return nodeId < m_Nodes.size() && !m_Empties.contains(nodeId);
            }

            TNodeId New()
            {
                TNodeId nodeId = GetEmpty();
                if (m_Nodes.size() > 0) {
                    Attach(nodeId, 0);
                    m_Heights[nodeId] = 1;
                }
                else m_Heights[nodeId] = 0;
                return nodeId;
            }

            TNodeId New(TNodeId parentId)
            {
                LV_CORE_ASSERT(Has(parentId), "Invalid parent ID!");

                TNodeId node = GetEmpty();
                Attach(node, parentId);
                return node;
            }

            Node const& Get(TNodeId nodeId)
            {
                LV_CORE_ASSERT(Has(nodeId), "Invalid node ID!");
                return m_Nodes[nodeId];
            }

            int Height(TNodeId nodeId)
            {
                LV_CORE_ASSERT(Has(nodeId), "Invalid node ID!");
                return m_Heights[nodeId];
            }

            void Remove(TNodeId nodeId)
            {
                if (nodeId == 0) Clear();
                else RecycleSubtree(nodeId);
            }

            void Clear()
            {
                m_Nodes.clear();
                m_Heights.clear();
                m_Empties.clear();
            }
        public:
            Node const& operator[](TNodeId nodeId)
            {
                LV_CORE_ASSERT(Has(nodeId), "Invalid node ID!");
                return m_Nodes[nodeId];
            }
        private:
            TNodeId GetEmpty()
            {
                TNodeId emptyId;
                if (m_Empties.empty()) {
                    emptyId = m_Nodes.size();
                    m_Nodes.emplace_back();
                    m_Heights.push_back(-1);
                }
                else {
                    auto it = m_Empties.begin();
                    emptyId = *it;
                    m_Empties.erase(it);
                }
                return emptyId;
            }

            void Recycle(TNodeId nodeId)
            {
                LV_CORE_ASSERT(Has(nodeId), "Invalid node ID!");
                m_Nodes[nodeId] = Node();
                m_Heights[nodeId] = -1;
                m_Empties.insert(nodeId);
            }

            void RecycleSubtree(TNodeId rootId)
            {
                LV_CORE_ASSERT(Has(rootId), "Invalid root node ID!");
                TNodeId firstId = m_Nodes[rootId].FirstChild;
                if (firstId != NNull) {
                    TNodeId childId = firstId;
                    do {
                        childId = m_Nodes[childId].NextSibling;
                        RecycleSubtree(m_Nodes[childId].PrevSibling);
                    } while (childId != firstId);
                }
                Recycle(rootId);
            }

            void Attach(TNodeId nodeId, TNodeId parentId)
            {
                auto& node = m_Nodes.Get(nodeId);
                auto& parent = m_Nodes.Get(parentId);

                // Connect to parent
                node.Parent = parentId;
                if (parent.FirstChild == NNull) {
                    parent.FirstChild = nodeId;
                    node.PrevSibling = node.NextSibling = nodeId;
                }
                else {
                    // Connect to siblings
                    auto& next = m_Nodes[parent.FirstChild];
                    auto& prev = m_Nodes[next.PrevSibling];

                    node.NextSibling = parent.FirstChild;
                    node.PrevSibling = next.PrevSibling;

                    next.PrevSibling = nodeId;
                    prev.NextSibling = nodeId;
                }

                m_Heights[nodeId] = m_Heights[parentId] + 1;
            }

            void Detach(TNodeId nodeId)
            {
                auto& node = m_Nodes.Get(nodeId);
                auto& parent = m_Nodes.Get(node.Parent);

                // Disconnect from parent
                if (parent.FirstChild == nodeId) {
                    parent.FirstChild = node.NextSibling == nodeId ? NNull : node.NextSibling;
                }
                node.Parent = NNull;

                // Disconnect from siblings
                if (node.NextSibling != nodeId)
                {
                    auto& next = m_Objects[node.NextSibling];
                    auto& prev = m_Objects[node.PrevSibling];
                    next.PrevSibling = node.PrevSibling;
                    prev.NextSibling = node.NextSibling;
                }
                node.NextSibling = node.PrevSibling = NNull;

                m_Heights[nodeId] = -1;
            }
        };

    private:
        using TAttrId = uint32_t;

        template<typename TAttr>
        class AttributeStorage
        {
            std::vector<TAttr> m_Attributes;
            std::unordered_set<TAttrId> m_Empties;
            std::unordered_map<TNodeId, TAttrId> m_NodeToAttr;
        public:
            AttributeStorage() = default;
            AttributeStorage(const AttributeStorage&) = default;

            bool Has(TNodeId nodeId)
            {
                return m_NodeToAttr.contains(nodeId);
            }

            TAttr& Add(TNodeId nodeId)
            {
                LV_CORE_ASSERT(!m_NodeToAttr.contains(nodeId), "Node already has attribute!");
                TAttrId attr = GetEmpty();
                m_NodeToAttr[nodeId] = attr;
                return m_Attributes[attr];
            }

            TAttr& Get(TNodeId nodeId)
            {
                LV_CORE_ASSERT(m_NodeToAttr.contains(nodeId), "Node is missing requested attribute!");
                return m_Attributes[m_NodeToAttr[nodeId]];
            }

            TAttr& GetOrAdd(TNodeId nodeId)
            {
                return m_NodeToAttr.contains(nodeId) ?
                    Add(nodeId) : Get(nodeId);
            }

            void Remove(TNodeId nodeId)
            {
                LV_CORE_ASSERT(m_NodeToAttr.contains(nodeId), "Node does not have the attribute to remove!");
                Recycle(m_NodeToAttr[nodeId]);
                m_NodeToAttr.erase(nodeId);
            }

            void TryRemove(TNodeId nodeId)
            {
                if (m_NodeToAttr.contains(nodeId)) {
                    Recycle(m_NodeToAttr[nodeId]);
                    m_NodeToAttr.erase(nodeId);
                }
            }
        public:
            TAttr& operator[](TNodeId nodeId)
            {
                LV_CORE_ASSERT(m_NodeToAttr.contains(nodeId), "Node is missing requested attribute!");
                return m_Attributes[m_NodeToAttr[nodeId]];
            }
        private:
            TAttrId GetEmpty()
            {
                TAttrId emptyAttr;
                if (m_Empties.empty()) {
                    emptyAttr = m_Attributes.size();
                    m_Attributes.emplace_back();
                }
                else {
                    auto it = m_Empties.begin();
                    emptyAttr = *it;
                    m_Empties.erase(it);
                }
                return emptyAttr;
            }

            void Recycle(TAttrId attributeId)
            {
                m_Empties.insert(attributeId);
            }
        };


        /*** Member objects ***/
    public:
        enum class Validity
        {
            InvalidParent   = 0,
            InvalidMass     = 1,
            InvalidPosition = 2,
            InvalidPath     = 3,
            Valid           = 100
        };

        enum class OrbitType
        {
            Circle    = 0,
            Ellipse   = 1,
            Hyperbola = 2
        };
    private:
        struct State
        {
            /* Required attribute - all physics objects are expected to have a physics state */
            double Mass = 0.0;
            Vector3 Position = { 0.f };
            Vector3d Velocity = { 0.0 };
            Vector3d Acceleration = { 0.0 };
        };

        struct Integration
        {
            enum class Method {
                Angular = 0,
                Linear = 1
            };

            double PrevDT = 0.0;
            double UpdateTimer = 0.0;
            float DeltaTrueAnomaly = 0.f;

            TNodeId UpdateNext = Null;
            Method Method = Method::Angular; /* set to Angular by default in TryComputeAttributes() anyway */
        };


        /*** Attribute objects ***/
    private:
        struct Object
        {
            TUserId UserId = TUserId();

            Validity Validity = Validity::InvalidParent;
            State State;
            Integration Integration;

            TNodeId Influence = NNull; /* Local space node representing this object's sphere of influence: Null if object is not influencing */

            Object() = default;
            Object(TUserId userId) : UserId(userId) {}
        };

        class LocalSpace
        {
        public:
            float Radius = 0.f; /* Measured in parent's influence */
            float MetersPerRadius = 0.f;

            TNodeId PrimaryLsp = NNull;
            bool Influencing = false; /* True if the parent object is the local dominant source of gravity, i.e, this LSP is less than or equal to the parent's influence LSP */
        };

    public:
        struct Elements
        {
            double Grav = 0.f;          /* Gravitational parameter (mu) */
            double H = 0.0;             /* Orbital specific angular momentum */
            float E = { 0.f };          /* Eccentricity */

            OrbitType Type = OrbitType::Circle; /* Type of orbit - defined by eccentricity, indicates the type of shape which describes the orbit path */

            float P = 0.f;              /* Orbit parameter, or semi-latus rectum:   h^2 / mu    */
            double VConstant = 0.f;      /* Constant factor of orbital velocity:     mu / h      */

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
            Vector2 EscapePointPerifocal = { 0.f }; /* The escape point relative to the perifocal frame - 2D because it is restricted to the orbital (perifocal-XY) plane */

            Vector3d ContAcceleration = { 0.0 }; /* Acceleration assumed to be constant between timesteps */
        };

        /*** Node wrappers ***/
    private:
        class LSpaceNode;
        class ObjectNode;
        {
            TNodeId m_NodeId;
        public:
            ObjectNode() : m_NodeId(NNull) {}
            ObjectNode(ObjectNode const&) = default;
            ObjectNode(TNodeId nodeId) : m_NodeId(nodeId) {
                LV_CORE_ASSERT(m_Tree.Has(nodeId) || nodeId == NNull, "Invalid ID!");
                LV_CORE_ASSERT(m_Tree.Height(nodeId) % 2 == 0, "Class is for object nodes only!");
                LV_CORE_ASSERT(m_Objects.Has(nodeId), "Object node must have an Object attribute!");
            }

            bool IsNull() { return m_NodeId == NNull; }
            TNodeId Id() { return m_NodeId; }
            Node& Node() { return m_Tree[m_NodeId]; }
            bool IsRoot() { return m_NodeId == kRootObjId; }
            int Height() { return m_Tree.Height(m_NodeId); }

            Object& Object() { return m_Objects[m_NodeId]; }
            Elements& Elements() { return m_Elements[m_NodeId]; }
            Dynamics& Dynamics() { return m_Dynamics[m_NodeId]; }
            bool IsDynamic() { return m_Dynamics.Has(m_NodeId); }

            static ObjectNode New(LSpaceNode parent)
            {
                TNodeId newNodeId = m_Tree.New(parent.Id());
                m_Objects.Add(newNodeId);
                m_Elements.Add(newNodeId);
                return ObjectNode{ newNodeId };
            }
            static void Remove(ObjectNode objNode)
            {
                m_Objects.Remove(objNode.Id());
                m_Elements.Remove(objNode.Id());
                m_Tree.Remove(objNode.Id());
            }

            LSpaceNode Parent() { return LSpaceNode{ m_Tree[m_NodeId].Parent }; }
            ObjectNode Grandparent() { return ObjectNode{ m_Tree[m_Tree[m_NodeId].Parent].Parent }; }

            LSpaceNode PrimaryLsp() { return LSpaceNode{ m_LSpaces[m_Tree[m_NodeId].Parent].PrimaryLsp }; }
            ObjectNode PrimaryObj() { return ObjectNode{ m_Tree[m_LSpaces[m_Tree[m_NodeId].Parent].PrimaryLsp].Parent }; }

            Vector3 LocalPositionFromPrimary()
            {
                return m_Objects[m_NodeId].State.Position + LSpaceNode(m_Tree[m_NodeId].Parent).LocalOffsetFromPrimary();
            }
        public:
            operator TNodeId() const { return m_NodeId; }
        public:
            bool operator==(ObjectNode const& rhs) const { return this->m_NodeId == rhs.m_NodeId; }
        };

        class LSpaceNode
        {
            TNodeId m_NodeId;
        public:
            LSpaceNode() : m_NodeId(NNull) {}
            LSpaceNode(LSpaceNode const&) = default;
            LSpaceNode(TNodeId nodeId) : m_NodeId(nodeId) {
                LV_CORE_ASSERT(m_Tree.Has(nodeId) || nodeId == NNull, "Invalid ID!");
                LV_CORE_ASSERT(m_Tree.Height(nodeId) % 2 == 1, "Class is for local space nodes only!");
                LV_CORE_ASSERT(m_LSPaces.Has(nodeId), "Local space node must have a LocalSpace attribute!");
            }

            TNodeId Id() { return m_NodeId; }
            Node& Node() { return m_Tree[m_NodeId]; }
            bool IsRoot() { return m_NodeId == kRootLspId; }
            int Height() { return m_Tree.Height(m_NodeId); }

            LocalSpace& LSpace() { return m_LSpaces[m_NodeId]; }

            static LSpaceNode New(ObjectNode parentNode)
            {
                TNodeId newNodeId = m_Tree.New(parentNode.Id());
                m_LSpaces.Add(newNodeId);
                return LSpaceNode{ newNodeId };
            }
            static void Remove(LSpaceNode lspNode)
            {
                m_LSpaces.Remove(lspNode.Id());
                m_Tree.Remove(lspNode.Id());
            }

            ObjectNode Parent() { return ObjectNode{ m_Tree[m_NodeId].Parent }; }
            LSpaceNode Grandparent() { return LSpaceNode{ m_Tree[m_Tree[m_NodeId].Parent].Parent }; }

            LSpaceNode PrimaryLsp() { return LSpaceNode{ m_LSpaces[m_NodeId].PrimaryLsp }; }
            ObjectNode PrimaryObj() { return ObjectNode{ m_Tree[m_LSpaces[m_NodeId].PrimaryLsp].Parent }; }

            Vector3 LocalOffsetFromPrimary() { return LocalOffsetFromPrimary(m_NodeId, m_LSpaces[m_NodeId].Primary); }
        public:
            bool operator==(ObjectNode const& rhs) { return this->m_NodeId == rhs.m_NodeId; }
        private:
            Vector3 LocalOffsetFromPrimary(TNodeId lspId, TNodeId primaryLspId)
            {
                LV_CORE_ASSERT(m_Tree.Height(lspId) % 2 == 1 && m_Tree.Height(primaryLspId) % 2 == 1, "Invalid IDs!");
                if (lspId == primaryLspId) return { 0 };

                TNodeId lspParentObjId = m_Tree[lspId].Parent;
                return (m_Objects[lspParentObjId].State.Position + LocalPositionFromPrimary(m_Tree[lspParentObjId].Parent, primaryLspId))
                    / m_LSpaces[lspId].Radius;
            }
        };

        /*** Simulation resources ***/
    private:
        Tree m_Tree;
        static constexpr TNodeId kRootObjId = 0;
        static constexpr TNodeId kRootLspId = 1;

        ObjectNode m_RootObjNode = { kRootObjId };
        LSpaceNode m_RootLspNode = { kRootLspId };

        AttributeStorage<Object> m_Objects;
        AttributeStorage<LocalSpace> m_LSpaces;
        AttributeStorage<Elements> m_Elements;
        AttributeStorage<Dynamics> m_Dynamics;

        ObjectNode m_UpdateQueueFront = { NNull };

        std::function<void(TUserId, TUserId)> m_ParentChangedCallback;

        /*** Simulation helpers ***/
    private:
        bool ValidPosition(ObjectNode objNode)
        {
            static constexpr float kEscapeDistanceSqr = kLocalSpaceEscapeRadius * kLocalSpaceEscapeRadius;

            return objNode.Object().State.Position.SqrMagnitude() < kEscapeDistanceSqr;
                /* TODO - check for influence overlaps */;
        }

        bool ValidMass(ObjectNode objNode)
        {
            static constexpr double kMaxCOG = 1e-4; /* Maximum offset for shared centre of gravity */

            auto& obj = objNode.Object();
            bool hasValidMass = obj.State.Mass > 0.0;
            if (objNode.IsRoot()) {
                // TODO - min/max root mass ?
            }
            else {
                hasValidMass = hasValidMass && kMaxCOG > obj.State.Mass / (obj.State.Mass + objNode.PrimaryObj().Object().State.Mass);
            }
            return hasValidMass;
        }

        bool ValidParent(ObjectNode objNode)
        {
            if (objNode.IsRoot()) return true;
            if (m_RootLspNode.LSpace().MetersPerRadius > 0.0) {
                return objNode.Grandparent().Object().Validity == Validity::Valid;
            }
            LV_WARN("OrbitalPhysics root scaling has not been set!");
            return false;
        }

        bool ComputeStateValidity(ObjectNode objNode)
        {
            // TODO : #ifdef LV_DEBUG ??? is validity needed in release?

            Validity validity = Validity::Valid;
            if (!ValidParent(objNode)) {
                validity = Validity::InvalidParent;
            }
            else if (!ValidMass(objNode)) {
                validity = Validity::InvalidMass;
            }
            else if (!ValidPosition(objNode)) {
                validity = Validity::InvalidPosition;
            }
            /* Currently ignores velocity - no invalid velocities */

            objNode.Object().Validity = validity;
            return validity == Validity::Valid;
        }

        inline float OrbitEquation(ObjectNode objNode, float trueAnomaly)
        {
            return objNode.Elements().P / (1.f + objNode.Elements().E * cosf(trueAnomaly));
        }

        Vector3 ObjectPositionAtTrueAnomaly(ObjectNode objNode, float trueAnomaly)
        {
            float radiusAtTrueAnomaly = OrbitEquation(objNode, trueAnomaly);
            Vector3 directionAtTrueAnomaly = cosf(trueAnomaly) * objNode.Elements().PerifocalX
                + sinf(trueAnomaly) * objNode.Elements().PerifocalY;
            Vector3 positionFromPrimary = radiusAtTrueAnomaly * directionAtTrueAnomaly;
            return positionFromPrimary - objNode.Parent().LocalOffsetFromPrimary();
        }

        void ComputeInfluence(ObjectNode objNode)
        {
            LV_CORE_ASSERT(!objNode.IsRoot(), "Cannot compute influence of root object!");

            Object& obj = objNode.Object();

            /* Radius of influence = a(m / M)^0.4
             * Semi-major axis must be in the order of 1,
             * so the order of ROI is determined by (m / M)^0.4 */
            static constexpr double kMinimumMassFactor = 1e-4; // TODO : test for optimal values
            double massFactor = pow(obj.State.Mass / objNode.PrimaryObj.Object().State.Mass, 0.4);
            if (massFactor > kMinimumMassFactor) {
                LSpaceNode lspNode = obj.Influence == NNull ? LSpaceNode::New(nodeId) : LSpaceNode{ obj.Influence };
                obj.Influence = lspId;

                LocalSpace& lsp = LSpaceNode.LSpace();
                lsp.Influencing = true;

                LSpaceNode primaryLspNode = lspNode.Grandparent();
                while (primaryLspNode.LSpace().Influencing == false) {
                    LV_CORE_ASSERT(primaryLspNode.Height() > 1, "This local space does not have a grandparent!");
                    primaryLspNode = primaryLspNode.Grandparent();
                }
                lsp.PrimaryLsp = primaryLspNode.Id();

                lsp.Radius = objNode.Elements().SemiMajor * massFactor;
                lsp.MetersPerRadius = objNode.Parent().LSpace().MetersPerRadius * lsp.Radius;
            }
            else if (obj.Influence != NNull) {
                // Object was previously influencing
                LSpaceNode inflNode = LSpaceNode{ obj.Influence };
                if (inflNode.Node().FirstChild == NNull) {
                    LSpaceNode::Remove(inflNode);
                }
                else {
                    inflNode.LSpace().Influencing = false;
                    LV_CORE_ASSERT(false, "TODO: re-primary children to this object's primary!");
                }
                obj.Influence = NNull;
            }
        }

        void ComputeDynamics(ObjectNode objNode)
        {
            LV_CORE_ASSERT(!objNode.IsRoot(), "Cannot compute dynamics on root object!");

            auto& obj = objNode.Object();
            auto& elems = objNode.Elements();

            float apoapsisRadius = elems.P / (1.f - elems.E);
            bool escapesLocalSpace = elems.Type == OrbitType::Hyperbola || apoapsisRadius > kLocalSpaceEscapeRadius;

            float escapeTrueAnomaly = 0.f;
            if (escapesLocalSpace) {
                escapeTrueAnomaly = acosf((elems.P / kLocalSpaceEscapeRadius - 1.f) / elems.E);
            }

            // TODO : #ifdef to exclude Validity from release ?
            LV_CORE_ASSERT(obj.Validity == Validity::Valid || obj.Validity == Validity::InvalidPath,
                "Cannot compute dynamics on object with invalid parent, mass, or position!");

            obj.Validity = Validity::Valid;
            if (objNode.IsDynamic())
            {
                if (escapesLocalSpace && objNode.Parent().IsRoot()) {
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

            // NOTE : above ELSE block always returns, so 'm_Dynamics.Get(nodeId)' is guaranteed to work here
            auto& dynamics = objNode.Dynamics();

            dynamics.EscapeTrueAnomaly = escapeTrueAnomaly;

            dynamics.EscapePoint = { 0.f };
            dynamics.EntryPoint = { 0.f };
            dynamics.EscapePointPerifocal = { 0.f };
            if (escapesLocalSpace)
            {
                float cosTEscape = cosf(escapeTrueAnomaly);
                float sinTEscape = sinf(escapeTrueAnomaly);

                float entryTrueAnomaly = PI2f - escapeTrueAnomaly;

                Vector3 escapeDirection = cosTEscape * elems.PerifocalX
                    + sinTEscape * elems.PerifocalY;
                Vector3 entryDirection = cosf(entryTrueAnomaly) * elems.PerifocalX
                    + sinf(entryTrueAnomaly) * elems.PerifocalY;

                dynamics.EscapePoint = kLocalSpaceEscapeRadius * escapeDirection;
                dynamics.EntryPoint = kLocalSpaceEscapeRadius * entryDirection;

                dynamics.EscapePointPerifocal = { kLocalSpaceEscapeRadius * cosTEscape - elems.C, /* subtract center's x-offset to convert x-component to the perifocal frame */
                    kLocalSpaceEscapeRadius * sinTEscape };
            }
        }

        void ComputeElements(ObjectNode objNode)
        {
            LV_CORE_ASSERT(!objNode.IsRoot(), "Cannot compute elements on root object!");

            auto& obj = objNode.Object();
            auto& elems = objNode.Elements();
            auto& lspNode = objNode.Parent();
            auto& lsp = lspNode.LSpace();

            LV_CORE_ASSERT(obj.Validity == Validity::Valid || obj.Validity == Validity::InvalidPath,
                "Cannot compute elements on an object with invalid parent, mass, or position!");

            elems.Grav = kGravitational * objNode.PrimaryObj().Object().State.Mass * pow(lsp.MetersPerRadius, -3.0);

            Vector3 positionFromPrimary = objNode.LocalPositionFromPrimary();

            Vector3d Hvec = Vector3d(positionFromPrimary).Cross(obj.State.Velocity);
            double H2 = Hvec.SqrMagnitude();
            elems.H = sqrt(H2);
            elems.PerifocalNormal = (Vector3)(Hvec / elems.H);

            /* Loss of precision due to casting is acceptable: semi-latus rectum is on the order of 1 in all common cases, due to distance parameterisation */
            elems.P = (float)(H2 / elems.Grav);
            elems.VConstant = elems.Grav / elems.H;

            /* Loss of precision due to casting is acceptable: result of vector division (V x H / Grav) is on the order of 1 */
            Vector3 posDir = positionFromPrimary.Normalized();
            Vector3 Evec = (Vector3)(obj.State.Velocity.Cross(Hvec) / elems.Grav) - posDir;
            elems.E = sqrtf(Evec.SqrMagnitude());
            float e2 = powf(elems.E, 2.f), e2term;
            if (elems.E < kEccentricityEpsilon)
            {
                // Circular
                elems.E = 0.f;
                elems.Type = OrbitType::Circle;

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
                    elems.Type = OrbitType::Ellipse;
                    e2term = 1.f - e2;
                }
                else {
                    // Hyperbolic
                    elems.Type = OrbitType::Hyperbola;
                    e2term = e2 - 1.f;
                }
                e2term += std::numeric_limits<float>::epsilon(); /* guarantees e2term > 0 */
            }

            // Dimensions
            elems.SemiMajor = elems.P / e2term;
            elems.SemiMinor = elems.SemiMajor * sqrtf(e2term);

            elems.C = elems.P / (1.f + elems.E);
            elems.C += (elems.Type == OrbitType::Hyperbola) ? elems.SemiMajor : -elems.SemiMajor; /* different center position for cirlce/ellipse and hyperbola */

            elems.T = PI2 * (double)(elems.SemiMajor * elems.SemiMinor) / elems.H;

            elems.TrueAnomaly = AngleBetweenUnitVectors(elems.PerifocalX, posDir);
            // Disambiguate based on whether the position is in the positive or negative Y-axis of the perifocal frame
            if (posDir.Dot(elems.PerifocalY) < 0.f) {
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
            elems.ArgPeriapsis = AngleBetweenUnitVectors(elems.N, elems.PerifocalX);
            if (elems.N.Dot(elems.PerifocalY) > 0.f) {
                elems.ArgPeriapsis = PI2f - elems.ArgPeriapsis;
            }
            elems.PerifocalOrientation =
                Quaternion(elems.PerifocalNormal, elems.ArgPeriapsis)
                * Quaternion(elems.N, elems.I)
                * Quaternion(kReferenceNormal, elems.Omega);
        }


        inline double ComputeObjDT(double velocityMagnitude, double minDT = kDefaultMinDT)
        {
            if (velocityMagnitude > 0) {
                return std::max(kMinUpdateDistanced / velocityMagnitude, minDT);
            }
            return minDT;
        }


        void TryComputeAttributes(ObjectNode objNode)
        {
            UpdateQueueSafeRemove(objNode);

            auto& obj = objNode.Object();
            if (!objNode.IsRoot() && (obj.Validity == Validity::Valid || obj.Validity == Validity::InvalidPath))
            {
                ComputeElements(objNode);
                ComputeDynamics(objNode); /* sets Validity to InvalidPath if dynamic events are found and orbiter is not dynamic */
                ComputeInfluence(objNode);

                if (obj.Validity == Validity::Valid)
                {
                    UpdateQueuePushFront(objNode);

                    obj.Integration.PrevDT = ComputeObjDT(sqrt(obj.State.Velocity.SqrMagnitude()));
                    Vector3 positionFromPrimary = objNode.LocalPositionFromPrimary();
                    float posMag2 = positionFromPrimary.SqrMagnitude();
                    obj.Integration.DeltaTrueAnomaly = (float)(obj.Integration.PrevDT * objNode.Elements().H) / posMag2;

                    // TODO : handle cases where dynamic acceleration is non-zero, e.g, bool isDynamicallyAccelerating = m_Dynamics.Has(object) && !m_Dynamics[object].ContAcceleration.IsZero()
                    if (obj.Integration.DeltaTrueAnomaly > kMinUpdateTrueAnomaly) {
                        obj.Integration.Method = Integration::Method::Angular;
                    }
                    else {
                        Vector3 posDir = positionFromPrimary / sqrtf(posMag2);
                        obj.State.Acceleration = -(Vector3d)posDir * objNode.Elements().Grav / (double)posMag2;
                        if (objNode.IsDynamic()) {
                            obj.State.Acceleration += objNode.Dynamics().ContAcceleration;
                        }
                        obj.Integration.Method = Integration::Method::Linear;
                    }
                }
            }
        }


        /// <summary>
        /// Returns speed for a circular orbit around the local primary (not circular in local space if local space is not influencing) at the given distance (measured in local space radii).
        /// Assumes orbiter has insignificant mass compared to primary.
        /// </summary>
        double CircularOrbitSpeed(LSpaceNode lspNode, float localRadius)
        {
            /* ||V_circular|| = sqrt(mu / ||r||), where mu is the gravitational parameter of the orbit */
            return sqrt(kGravitational * lspNode.PrimaryObj.Object().State.Mass * pow(lspNode.LSpace().MetersPerRadius, -3.0) / (double)localRadius);
        }


        /// <summary>
        /// Returns velocity for a circular counter-clockwise orbit around the given primary at the given position.
        /// Assumes orbiter has insignificant mass compared to primary.
        /// </summary>
        /// <param name="object">Physics object ID</param>
        Vector3d CircularOrbitVelocity(LSpaceNode lspNode, Vector3 const& localPosition)
        {
            /* Keep the orbital plane as flat (close to the reference plane) as possible:
             * derive velocity direction as the cross product of reference normal and normalized position */
            Vector3d vDir;
            Vector3 positionFromPrimary = localPosition + lspNode.LocalOffsetFromPrimary();
            float rMag = sqrtf(positionFromPrimary.SqrMagnitude());
            Vector3 rDir = positionFromPrimary / rMag;

            float rDotNormal = rDir.Dot(kReferenceNormal);
            if (abs(rDotNormal) > kParallelDotProductLimit) {
                /* Handle cases where the normal and position are parallel:
                 * counter-clockwise around the reference Y-axis, whether above or below the plane */
                vDir = rDotNormal > 0.f ? (Vector3d)(-kReferenceX) : (Vector3d)kReferenceX;
            }
            else {
                vDir = (Vector3d)kReferenceNormal.Cross(rDir).Normalized();
            }
            return vDir * CircularOrbitSpeed(lspNode, rMag);
        }


        void UpdateQueuePushFront(ObjectNode objNode)
        {
            if (m_UpdateQueueFront.IsNull()) {
                m_UpdateQueueFront = objNode;
                objNode.Object().Integration.UpdateNext = NNull;
            }
            else {
                objNode.Object().Integration.UpdateNext = m_UpdateQueueFront;
                m_UpdateQueueFront = objNode;
            }
        }

        /// <summary>
        /// Removes the given object from the update queue.
        /// Attempting to remove an object which is not in the queue will result in an array out-of-bounds error caused by executing 'm_Objects[Null]'.
        /// See also: UpdateQueueSafeRemove().
        /// </summary>
        void UpdateQueueRemove(TObjectId object)
        {
            LV_CORE_ASSERT(m_UpdateQueueFront != Null, "Attempting to remove item from empty queue!");
            if (m_UpdateQueueFront == object) {
                m_UpdateQueueFront = m_Objects[object].Integration.UpdateNext;
                m_Objects[object].Integration.UpdateNext = Null;
                return;
            }
            TObjectId queueItem = m_UpdateQueueFront,
                queueNext = m_Objects[m_UpdateQueueFront].Integration.UpdateNext;
            while (queueNext != object) {
                LV_CORE_ASSERT(queueNext != Null, "UpdateQueueRemove() could not find the given object in the update queue!");
                queueItem = queueNext;
                queueNext = m_Objects[queueNext].Integration.UpdateNext;
            }
            m_Objects[queueItem].Integration.UpdateNext = m_Objects[object].Integration.UpdateNext;
            m_Objects[object].Integration.UpdateNext = Null;
        }

        /// <summary>
        /// Removes the given object from the update queue, if it exists in the update queue.
        /// </summary>
        /// <returns>True if object was found and removed, false otherwise.</returns>
        bool UpdateQueueSafeRemove(TObjectId object)
        {
            if (m_UpdateQueueFront == Null) return false;
            if (m_UpdateQueueFront == object) {
                m_UpdateQueueFront = m_Objects[object].Integration.UpdateNext;
                m_Objects[object].Integration.UpdateNext = Null;
                return true;
            }
            TObjectId queueItem = m_UpdateQueueFront,
                queueNext = m_Objects[m_UpdateQueueFront].Integration.UpdateNext;
            while (queueNext != Null) {
                if (queueNext == object) {
                    m_Objects[queueItem].Integration.UpdateNext = m_Objects[queueNext].Integration.UpdateNext;
                    m_Objects[object].Integration.UpdateNext = Null;
                    return true;
                }
                queueItem = queueNext;
                queueNext = m_Objects[queueNext].Integration.UpdateNext;
            }
            return false;
        }

        /// <summary>
        /// Assumes the first entry in the queue is the only entry which is potentially unsorted.
        /// </summary>
        void UpdateQueueSortFront()
        {
            LV_CORE_ASSERT(m_UpdateQueueFront != Null, "Attempting to sort empty queue!");

            TObjectId object = m_UpdateQueueFront;
            auto& integ = m_Objects[object].Integration;

            TObjectId queueItem = integ.UpdateNext;
            if (queueItem == Null) return;
            if (integ.UpdateTimer < m_Objects[queueItem].Integration.UpdateTimer) return;
            m_UpdateQueueFront = queueItem;

            TObjectId queueNext = m_Objects[queueItem].Integration.UpdateNext;
            while (queueNext != Null) {
                if (integ.UpdateTimer < m_Objects[queueNext].Integration.UpdateTimer) break;
                queueItem = queueNext;
                queueNext = m_Objects[queueNext].Integration.UpdateNext;
            }
            m_Objects[queueItem].Integration.UpdateNext = object;
            integ.UpdateNext = queueNext;
        }


        /// <summary>
        /// Appends children of 'object' to 'children'.
        /// </summary>
        /// <returns>Number of children</returns>
        size_t GetObjectChildren(std::vector<TObjectId>& children, TObjectId object)
        {
            size_t numChildren = 0;
            TObjectId first = m_LocalSpaces.Has(object) ? m_LocalSpaces.Get(object).FirstChild : Null;
            if (first != Null) {
                TObjectId child = first;
                do {
                    numChildren++;
                    children.push_back(child);
                    child = m_Objects[child].NextSibling;
                    LV_CORE_ASSERT(child != Null, "Sibling circular-linked list is broken!");
                } while (child != first);
            }
            return numChildren;
        }


        /// <summary>
        /// Performs a breadth-first search of the entire tree beginning at 'root' and appends results to 'tree'.
        /// Does NOT append 'root' to 'tree'.
        /// </summary>
        void GetObjectTree(std::vector<TObjectId>& tree, TObjectId root)
        {
            size_t numAdded = GetObjectChildren(tree, root);
            do {
                size_t end = tree.size();
                size_t idx = end - numAdded;
                numAdded = 0;
                while (idx < end) {
                    numAdded += GetObjectChildren(tree, tree[idx]);
                    idx++;
                }
            } while (numAdded > 0);
        }


        void TreeCascadeAttributeChanges(TObjectId object)
        {
            std::vector<TObjectId> tree = {};
            GetObjectTree(tree, object);
            for (auto obj : tree) {
                // TODO : preserve orbit shape ?
                if (ComputeStateValidity(obj)) {
                    TryComputeAttributes(obj);
                }
            }
        }


        void ChangeParentAtRuntime(TObjectId object, TObjectId newParent, TUserId objectUser, TUserId newParentUser)
        {
            DetachObject(object);
            AttachObject(object, newParent);

            if (m_ParentChangedCallback) {
                m_ParentChangedCallback(objectUser, newParentUser);
            }
        }

    public:
        /*** Usage ***/

#ifdef LV_DEBUG
        struct ObjStats
        {
            size_t NumObjectUpdates = 0;
            std::chrono::duration<double> LastOrbitDuration = std::chrono::duration<double>::zero();
            double LastOrbitDurationError = 0;
        };
        struct Stats
        {
            std::vector<ObjStats> ObjStats;
            std::chrono::duration<double> UpdateTime = std::chrono::duration<double>::zero();
        };
        Stats m_Stats;
        Stats const& GetStats() { return m_Stats; }
#endif


        void OnUpdate(Timestep dT)
        {
#ifdef LV_DEBUG // debug pre-update
            std::chrono::steady_clock::time_point updateStart = std::chrono::steady_clock::now();
            static std::vector<std::chrono::steady_clock::time_point> timesOfLastPeriapsePassage = { };
            timesOfLastPeriapsePassage.resize(m_Objects.size(), std::chrono::steady_clock::time_point::min());
            for (ObjStats& stats : m_Stats.ObjStats) {
                stats.NumObjectUpdates = 0;
            }
            m_Stats.ObjStats.resize(m_Objects.size(), ObjStats());
#endif

            double minObjDT = dT / kMaxObjectUpdates;

            // Update all objects with timers less than 0
            while (m_Objects[m_UpdateQueueFront].Integration.UpdateTimer < 0.0)
            {
                auto& obj = m_Objects[m_UpdateQueueFront];
                auto& elems = m_Elements[m_UpdateQueueFront];
                bool isDynamic = m_Dynamics.Has(m_UpdateQueueFront);

#ifdef LV_DEBUG // debug object pre-update
                m_Stats.ObjStats[m_UpdateQueueFront].NumObjectUpdates += 1;
                float prevTrueAnomaly = elems.TrueAnomaly;
#endif

                double& objDT = obj.Integration.PrevDT;

                // Motion integration
                switch (obj.Integration.Method)
                {
                case Integration::Method::Angular:
                {
                    /* Integrate true anomaly:
                    * dTrueAnomaly / dT = h / r^2
                    * */
                    if (obj.Integration.DeltaTrueAnomaly < kMinUpdateTrueAnomaly) {
                        Vector3 positionFromPrimary = GetObjectPositionFromPrimary(nodeId);
                        float posMag2 = positionFromPrimary.SqrMagnitude();
                        Vector3 posDir = positionFromPrimary / sqrtf(posMag2);
                        obj.State.Acceleration = -(Vector3d)posDir * elems.Grav / (double)posMag2;
                        if (isDynamic) {
                            obj.State.Acceleration += m_Dynamics[m_UpdateQueueFront].ContAcceleration;
                        }
                        obj.Integration.Method = Integration::Method::Linear;
                        LV_CORE_TRACE("Object (UserId={0}) switched from angular to linear integration!", obj.UserId);
                        // NOTE: switch falls through to case Integration::Method::Linear
                    }
                    else {
                        elems.TrueAnomaly += obj.Integration.DeltaTrueAnomaly;
                        elems.TrueAnomaly = Wrapf(elems.TrueAnomaly, PI2f);

                        // Compute new state
                        float sinT = sinf(elems.TrueAnomaly);
                        float cosT = cosf(elems.TrueAnomaly);
                        float r = elems.P / (1.f + elems.E * cosT); /* orbit equation: r = h^2 / mu * 1 / (1 + e * cos(trueAnomaly)) */

                        Vector3 positionFromPrimary = r * (cosT * elems.PerifocalX + sinT * elems.PerifocalY);
                        obj.State.Position = GetOrbitalPositionFromParent(positionFromPrimary);
                        obj.State.Velocity = elems.VConstant * (Vector3d)((elems.E + cosT) * elems.PerifocalY - sinT * elems.PerifocalX);

                        objDT = ComputeObjDT(sqrt(obj.State.Velocity.SqrMagnitude()), minObjDT);
                        obj.Integration.DeltaTrueAnomaly = (float)(objDT * elems.H) / (r * r);
                        break;
                    }
                    // NOTE: case break is conditional on purpose!
                }
                case Integration::Method::Linear:
                {
                    /* Velocity verlet :
                    * p1 = p0 + v0 * dT + 0.5 * a0 * dT^2
                    * a1 = (-rDirection) * G * M / r^2 + dynamicAcceleration
                    * v1 = v0 + 0.5 * (a0 + a1) * dT
                    * */
                    obj.State.Position += (Vector3)(obj.State.Velocity * objDT) + 0.5f * (Vector3)(obj.State.Acceleration * objDT * objDT);
                    Vector3 positionFromPrimary = GetObjectPositionFromPrimary(nodeId);
                    float r2 = positionFromPrimary.SqrMagnitude();
                    Vector3d newAcceleration = -(Vector3d)positionFromPrimary * elems.Grav / (double)(r2 * sqrtf(r2));
                    bool isDynamicallyAccelerating = false;
                    if (isDynamic) {
                        newAcceleration += m_Dynamics[m_UpdateQueueFront].ContAcceleration;
                        isDynamicallyAccelerating = !m_Dynamics[m_UpdateQueueFront].ContAcceleration.IsZero();
                    }
                    obj.State.Velocity += 0.5 * (obj.State.Acceleration + newAcceleration) * objDT;
                    obj.State.Acceleration = newAcceleration;

                    if (isDynamicallyAccelerating) {
                        ComputeElements(m_UpdateQueueFront);
                        ComputeDynamics(m_UpdateQueueFront);
                        ComputeInfluence(m_UpdateQueueFront);
                    }
                    else {
                        // Update true anomaly with new position vector
                        // Code taken from ComputeElements():
                        Vector3 posDir = positionFromPrimary.Normalized();
                        float newTrueAnomaly = AngleBetweenUnitVectors(elems.PerifocalX, posDir);
                        if (posDir.Dot(elems.PerifocalY) < 0.f) {
                            newTrueAnomaly = PI2f - newTrueAnomaly;
                        }

                        // Not dynamically accelerating, so we ensure true anomaly does not decrease:
                        float dTrueAnomaly = newTrueAnomaly - elems.TrueAnomaly;
                        if (dTrueAnomaly < -PIf) {
                            elems.TrueAnomaly = newTrueAnomaly; /* True anomaly has wrapped around at periapsis in the forwards direction */
                        }
                        else if (!(dTrueAnomaly > PIf)) {
                            elems.TrueAnomaly = std::max(newTrueAnomaly, elems.TrueAnomaly); /* True anomaly has NOT wrapped at periapsis in the backwards direction(we can safely take the larger value) */
                        } /* else, true anomaly has wrapped backwards at periapsis so we discard the new value */
                    }

                    // Recheck integration method choice
                    objDT = ComputeObjDT(sqrt(obj.State.Velocity.SqrMagnitude()), minObjDT); //std::max(kMinUpdateDistanced / sqrt(obj.State.Velocity.SqrMagnitude()), minObjDT);
                    if (!isDynamicallyAccelerating)
                    {
                        obj.Integration.DeltaTrueAnomaly = (float)(objDT * elems.H) / positionFromPrimary.SqrMagnitude();
                        if (obj.Integration.DeltaTrueAnomaly > kMinUpdateTrueAnomaly) {
                            obj.Integration.Method = Integration::Method::Angular;
                        }
                    }

                    break;
                }
                }

#ifdef LV_DEBUG // debug object post-update
                if (elems.TrueAnomaly < prevTrueAnomaly) {
                    auto timeOfPeriapsePassage = std::chrono::steady_clock::now();
                    if (timesOfLastPeriapsePassage[m_UpdateQueueFront] != std::chrono::steady_clock::time_point::min()) {
                        m_Stats.ObjStats[m_UpdateQueueFront].LastOrbitDuration = timeOfPeriapsePassage - timesOfLastPeriapsePassage[m_UpdateQueueFront];
                        m_Stats.ObjStats[m_UpdateQueueFront].LastOrbitDurationError = (elems.T - m_Stats.ObjStats[m_UpdateQueueFront].LastOrbitDuration.count()) / elems.T;
                    }
                    timesOfLastPeriapsePassage[m_UpdateQueueFront] = timeOfPeriapsePassage;
                }
#endif

                // Test for orbit events
                if (isDynamic) {
                    auto& dynamics = m_Dynamics[m_UpdateQueueFront];

                    float escapeTrueAnomaly = dynamics.EscapeTrueAnomaly;
                    if (escapeTrueAnomaly > 0.f && elems.TrueAnomaly < PIf && elems.TrueAnomaly > escapeTrueAnomaly) {
                        LV_CORE_ASSERT(sqrtf(obj.State.Position.SqrMagnitude()) > kLocalSpaceEscapeRadius, "False positive on escape test!");
                        LV_CORE_ASSERT(obj.Parent != kRootObjId, "Cannot escape root local space!");

                        auto& localspace = m_LocalSpaces[m_UpdateQueueFront];
                        auto& oldParentObj = m_Objects[obj.Parent];

                        float rescalingFactor = m_LocalSpaces[obj.Parent].Radius;
                        obj.State.Position = (obj.State.Position * rescalingFactor) + oldParentObj.State.Position;
                        obj.State.Velocity = (obj.State.Velocity * (double)rescalingFactor) + oldParentObj.State.Velocity;
                        if (!localspace.Influencing) {
                            localspace.Radius *= rescalingFactor; /* preserves absolute radius of local space */
                        }

                        ChangeParentAtRuntime(m_UpdateQueueFront, oldParentObj.Parent, obj.UserId, m_Objects[oldParentObj.Parent].UserId);

                        ComputeElements(m_UpdateQueueFront);
                        ComputeDynamics(m_UpdateQueueFront);
                        ComputeInfluence(m_UpdateQueueFront);
                        LV_CORE_ASSERT(obj.Validity == Validity::Valid, "Invalid dynamics after escape!");

                        objDT = ComputeObjDT(sqrt(obj.State.Velocity.SqrMagnitude()), minObjDT);
                        float posMag2 = obj.State.Position.SqrMagnitude();
                        obj.Integration.DeltaTrueAnomaly = (float)(objDT * elems.H) / posMag2;

                        // TODO : handle cases where dynamic acceleration is non-zero, e.g, bool isDynamicallyAccelerating = m_Dynamics.Has(object) && !m_Dynamics[object].ContAcceleration.IsZero()
                        if (obj.Integration.DeltaTrueAnomaly > kMinUpdateTrueAnomaly) {
                            LV_CORE_ASSERT(dynamics.ContAcceleration.IsZero(), "Dynamic acceleration not handled!");
                            obj.Integration.Method = Integration::Method::Angular;
                        }
                        else {
                            Vector3 posDir = obj.State.Position / sqrtf(posMag2);
                            obj.State.Acceleration = -(Vector3d)posDir * elems.Grav / (double)posMag2;
                            obj.State.Acceleration += dynamics.ContAcceleration;
                            obj.Integration.Method = Integration::Method::Linear;
                        }
                    }
                }

                obj.Integration.UpdateTimer += objDT;
                UpdateQueueSortFront();
            }

            // Subtract elapsed time from all object timers
            TObjectId object = m_UpdateQueueFront;
            do { 
                m_Objects[object].Integration.UpdateTimer -= dT;
                object = m_Objects[object].Integration.UpdateNext;
            } while (object != Null);

#ifdef LV_DEBUG // debug post-update
            m_Stats.UpdateTime = std::chrono::steady_clock::now() - updateStart;
#endif
        }


        void SetParentChangedCallback(std::function<void(TUserId, TUserId)> callback = {})
        {
            m_ParentChangedCallback = callback;
            if (!callback) LV_WARN("Callback function 'ParentChangedCallback' set to empty function!");
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
            m_Objects[kRootObjId].UserId = userRootId;
            return kRootObjId;
        }

        /// <summary>
        /// Sets scaling of root orbital space.
        /// Scaling is in meters per simulation length-unit.
        /// E.g, position vector with magnitude 1 in the root orbital space has a simulated magnitude equal to the root scaling.
        /// </summary>
        /// <param name="meters"></param>
        void SetRootScaling(double meters)
        {
            m_LocalSpaces[kRootObjId].MetersPerRadius = meters;
            TreeCascadeAttributeChanges(kRootObjId);
        }

        double GetRootScaling()
        {
            return m_LocalSpaces[kRootObjId].MetersPerRadius;
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
        /// Returns the user object associated with a given physics object.
        /// </summary>
        /// <param name="object">The ID of the physics object in question.</param>
        /// <returns>The user ID associated with the physics object.</returns>
        TUserId GetUser(TObjectId object)
        {
            return m_Objects[object].UserId;
        }


        /// <summary>
        /// Create an orbital physics object in the specified orbital space.
        /// </summary>
        /// <param name="userId">ID of the user-object to be associated with the created physics object</param>
        /// <returns>ID of the created physics object</returns>
        TObjectId Create(TUserId userId, TObjectId parent, double mass, Vector3 const& position, Vector3d const& velocity, bool dynamic = false)
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
            TryComputeAttributes(newObject);

            return newObject;
        }

        /// <summary>
        /// Create an orbital physics object in the specified orbital space.
        /// New object's velocity defaults to that of a circular orbit.
        /// </summary>
        /// <param name="userId">ID of the user-object to be associated with the created physics object</param>
        /// <returns>ID of the created physics object</returns>
        TObjectId Create(TUserId userId, TObjectId parent, bool mass, Vector3 const& position, bool dynamic = false)
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
            return Create(userId, kRootObjId, 0.0, { 0.f }, { 0.0 }, dynamic);
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

            DetachObject(object);
            AttachObject(object, parent);

            ComputeStateValidity(object);
            TryComputeAttributes(object);
            TreeCascadeAttributeChanges(object);
        }

        TUserId GetParent(TObjectId object)
        {
            return m_Objects[m_Objects[object].Parent].UserId;
        }

        std::vector<TUserId> GetChildren(TObjectId object)
        {
            std::vector<TUserId> children;
            TObjectId first = m_LocalSpaces.Has(object) ? m_LocalSpaces.Get(object).FirstChild : Null;
            if (first != Null) {
                TObjectId child = first;
                do {
                    children.push_back(m_Objects[child].UserId);
                    child = m_Objects[child].NextSibling;
                    LV_CORE_ASSERT(child != Null, "Sibling circular-linked list is broken!");
                } while (child != first);
            }
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
            if (!IsInfluencing(object)
                && radius < kMaxLocalSpaceRadius + kEpsLocalSpaceRadius
                && radius > kMinLocalSpaceRadius - kEpsLocalSpaceRadius)
            {
                m_LocalSpaces.Get(object).Radius = radius;
                TreeCascadeAttributeChanges(object);
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
            TryComputeAttributes(object); /* NOTE: this should be redundant as orbital motion is independent of orbiter mass, but do it anyway just for consistency */
            TreeCascadeAttributeChanges(object);
        }

        double GetMass(TObjectId object)
        {
            return m_Objects[object].State.Mass;
        }


        void SetPosition(TObjectId object, Vector3 const& position)
        {
            if (object == kRootObjId) {
                LV_ERROR("Cannot set position of OrbitalPhysics root object!");
                return;
            }

            m_Objects[object].State.Position = position;
            ComputeStateValidity(object);
            TryComputeAttributes(object);
            TreeCascadeAttributeChanges(object);
        }

        Vector3 const& GetPosition(TObjectId object)
        {
            return m_Objects[object].State.Position;
        }


        void SetVelocity(TObjectId object, Vector3d const& velocity)
        {
            if (object == kRootObjId) {
                LV_WARN("Cannot set velocity of OrbitalPhysics root object!");
                return;
            }

            m_Objects[object].State.Velocity = velocity;
            TryComputeAttributes(object);
            TreeCascadeAttributeChanges(object);
        }

        Vector3d const& GetVelocity(TObjectId object)
        {
            return m_Objects[object].State.Velocity;
        }

        /// <summary>
        /// Returns velocity for a circular counter-clockwise orbit around the object's current primary, given its current mass and position.
        /// </summary>
        /// <param name="object">Physics object ID</param>
        Vector3d GetCircularOrbitVelocity(TObjectId object)
        {
            /* Keep the orbital plane as flat (close to the reference plane) as possible:
             * derive velocity direction as the cross product of reference normal and normalized position */
            Vector3d vDir;
            float rMag = sqrtf(m_Objects[object].State.Position.SqrMagnitude());
            Vector3 rDir = m_Objects[object].State.Position / rMag;

            float rDotNormal = rDir.Dot(kReferenceNormal);
            if (abs(rDotNormal) > kParallelDotProductLimit) {
                /* Handle cases where the normal and position are parallel:
                 * counter-clockwise around the reference Y-axis, whether above or below the plane */
                vDir = rDotNormal > 0.f ? -kReferenceX : kReferenceX;
            }
            else {
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
            LV_CORE_ASSERT(object != kRootObjId, "Cannot set root object dynamics!");
            if (isDynamic) {
                m_Dynamics.GetOrAdd(object);
            }
            else {
                m_Dynamics.TryRemove(object);
            }
            TryComputeAttributes(object);
        }

        bool IsDynamic(TObjectId object)
        {
            return m_Dynamics.Has(object);
        }

        const Dynamics& GetDynamics(TObjectId object)
        {
            return m_Dynamics[object];
        }


        void SetContinuousAcceleration(TObjectId object, Vector3d const& acceleration, double dT = 1.0 / 60.0)
        {
            // TODO : test if calling this function before the simulation is run (e.g, from the editor in edit mode) does anything bad !!

            LV_CORE_ASSERT(m_Dynamics.Has(object), "Attempted to set dynamic acceleration on a non-dynamic orbiter!");

            auto& state = m_Objects[object].State;
            auto& dynamics = m_Dynamics[object];

            state.Position += 0.5f * (acceleration - dynamics.ContAcceleration) * powf(state.UpdateTimer, 2.f);
            state.Velocity += (acceleration - dynamics.ContAcceleration) * state.UpdateTimer;
            double newObjDT = ComputeObjDT(sqrt(state.Velocity.SqrMagnitude()));
            state.UpdateTimer += newObjDT - state.PrevDT; //max(kMinUpdateDistanced / state.Velocity, kDefaultMinDT) - state.PrevDT;
            state.PrevDT = newObjDT; // VERY UNTESTED !!!!
            dynamics.ContAcceleration = acceleration;

            LV_CORE_ASSERT(false, "Untested!");

            TryComputeAttributes(object);
        }

        void SetContinuousThrust(TObjectId object, Vector3d const& force)
        {
            SetContinuousAcceleration(object, force / m_Objects[object].State.Mass);
        }

        void ApplyInstantAcceleration(TObjectId object, Vector3d const& acceleration)
        {
            LV_CORE_ASSERT(false, "Not implemented!");
        }


        bool IsIntegrationLinear(TObjectId object)
        {
            return m_Objects[object].Integration.Method == Integration::Method::Linear;
        }

    };

}
