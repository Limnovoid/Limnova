#pragma once

#include <Math/Math.h>
#include <Core/Timestep.h>


namespace Limnova
{
    class OrbitalPhysics
    {
    public:
        OrbitalPhysics() = delete;

    private:
        /* Basis of the reference frame: the XY-plane represents the orbital plane of the system which has the root object as its primary */
        static constexpr Vector3 kReferenceX        = { 1.f, 0.f, 0.f };
        static constexpr Vector3 kReferenceY        = { 0.f, 0.f,-1.f };
        static constexpr Vector3 kReferenceNormal   = { 0.f, 1.f, 0.f };

        static constexpr double kGravitational = 6.6743e-11;


        // Simulation tuning parameters ////////
        // TODO : choose numbers based on reasoning/testing
        static constexpr float kDefaultLSpaceRadius = 0.1f;
        static constexpr float kLocalSpaceEscapeRadius = 1.01f;

        static constexpr float kEccentricityEpsilon = 1e-4f;

        static constexpr float kMaxLSpaceRadius = 0.2f;
        static constexpr float kMinLSpaceRadius = 0.01f;
        static constexpr float kEpsLSpaceRadius = 1e-6f;

        static constexpr float kMaxObjectUpdates = 20.f;
        static constexpr double kDefaultMinDT = 1.0 / (60.0 * kMaxObjectUpdates);
        static constexpr float kMinUpdateDistance = 1e-5f;
        static constexpr double kMinUpdateDistanced = (double)kMinUpdateDistance;
        static constexpr float kMinUpdateTrueAnomaly = 1e-5f;
        ////////////////////////////////////////


        /*** Object tree ***/
    public:
        using TNodeId = uint32_t;

        static constexpr TNodeId NNull = std::numeric_limits<TNodeId>::max();
    private:
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

            size_t Size()
            {
                return m_Nodes.size() - m_Empties.size();
            }

            bool Has(TNodeId nodeId) const
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

            Node const& Get(TNodeId nodeId) const
            {
                LV_CORE_ASSERT(Has(nodeId), "Invalid node ID!");
                return m_Nodes[nodeId];
            }

            int Height(TNodeId nodeId) const
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

            void Move(TNodeId nodeId, TNodeId newParentId)
            {
                Detach(nodeId);
                Attach(nodeId, newParentId);
            }

            size_t GetChildren(TNodeId nodeId, std::vector<TNodeId>& children) const
            {
                LV_CORE_ASSERT(Has(nodeId), "Invalid node ID!");
                size_t numChildren = 0;
                TNodeId first = m_Nodes[nodeId].FirstChild;
                if (first != NNull) {
                    TNodeId child = first;
                    do {
                        numChildren++;
                        children.push_back(child);
                        child = m_Nodes[child].NextSibling;
                        LV_CORE_ASSERT(child != NNull, "Sibling circular-linked list is broken!");
                    } while (child != first);
                }
                return numChildren;
            }

            size_t GetSubtree(TNodeId rootNodeId, std::vector<TNodeId>& subtree) const
            {
                size_t numAdded = GetChildren(rootNodeId, subtree);
                size_t sizeSubtree = numAdded;
                do {
                    size_t end = subtree.size();
                    size_t idx = end - numAdded;
                    numAdded = 0;
                    while (idx < end) {
                        numAdded += GetChildren(subtree[idx], subtree);
                        idx++;
                    }
                    sizeSubtree += numAdded;
                } while (numAdded > 0);
                return sizeSubtree;
            }

            TNodeId GetParent(TNodeId nodeId)
            {
                LV_CORE_ASSERT(Has(nodeId), "Invalid node ID!");
                LV_CORE_ASSERT(m_Heights[nodeId] > 0, "Cannot get parent of root node!");
                return m_Nodes[nodeId].Parent;
            }

            TNodeId GetGrandparent(TNodeId nodeId)
            {
                LV_CORE_ASSERT(Has(nodeId), "Invalid node ID!");
                LV_CORE_ASSERT(m_Heights[nodeId] > 1, "Node height is too low - node does not have a grandparent!");
                return m_Nodes[nodeId].Parent;
            }
        public:
            Node const& operator[](TNodeId nodeId) const
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
                auto& node = m_Nodes[nodeId];
                auto& parent = m_Nodes[parentId];

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
                auto& node = m_Nodes[nodeId];
                auto& parent = m_Nodes[node.Parent];

                // Disconnect from parent
                if (parent.FirstChild == nodeId) {
                    parent.FirstChild = node.NextSibling == nodeId ? NNull : node.NextSibling;
                }
                node.Parent = NNull;

                // Disconnect from siblings
                if (node.NextSibling != nodeId)
                {
                    auto& next = m_Nodes[node.NextSibling];
                    auto& prev = m_Nodes[node.PrevSibling];
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

            size_t Size()
            {
                return m_NodeToAttr.size();
            }

            bool Has(TNodeId nodeId) const
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

            TAttr& Get(TNodeId nodeId) const
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
            TAttr& operator[](TNodeId nodeId) const
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


        /*** Simulation classes
         Below this point, everything is explicitly for the physics simulation (for both internal-use and user-application-use)
         ***/

        /*** Node wrappers ***/
    private:
        static bool IsLocalSpace(TNodeId nodeId)
        {
            return m_Ctx->m_Tree.Height(nodeId) % 2; /* 0 -> object, 1 -> local space */
        }

    private:
        class LSpaceNode;

        class Object;
        class LocalSpace;
        class Elements;
        class Dynamics;
    public:
        class ObjectNode
        {
            friend class OrbitalPhysics;

            TNodeId m_NodeId;
        public:
            constexpr ObjectNode() : m_NodeId(OrbitalPhysics::NNull) {}
            ObjectNode(ObjectNode const&) = default;
            ObjectNode(TNodeId nodeId) : m_NodeId(nodeId)
            {
                if (nodeId != OrbitalPhysics::NNull) {
                    LV_CORE_ASSERT(m_Ctx->m_Tree.Has(nodeId), "Invalid ID!");
                    LV_CORE_ASSERT(m_Ctx->m_Tree.Height(nodeId) % 2 == 0, "Class is for object nodes only!");
                    LV_CORE_ASSERT(m_Ctx->m_Objects.Has(nodeId), "Object node must have an Object attribute!");
                    LV_CORE_ASSERT(m_Ctx->m_Elements.Has(nodeId), "Object node must have an Elements attribute!");
                }
            }

            TNodeId Id() const { return m_NodeId; }

            /* For OrbitalPhysics/internal use*/
        private:
            Node const& Node() const { return m_Ctx->m_Tree[m_NodeId]; }
            int Height() const { return m_Ctx->m_Tree.Height(m_NodeId); }
            Object& Object() const { return m_Ctx->m_Objects[m_NodeId]; }
            Elements& Elements() const { return m_Ctx->m_Elements[m_NodeId]; }
            Dynamics& Dynamics() const { return m_Ctx->m_Dynamics[m_NodeId]; }

            /* For user application/external use */
        public:
            static constexpr ObjectNode NNull() { return {}; }
            bool IsNull() const { return m_NodeId == OrbitalPhysics::NNull; }
            bool IsRoot() const { return m_NodeId == kRootObjId; }
            bool IsDynamic() const { return m_Ctx->m_Dynamics.Has(m_NodeId); }
            bool IsInfluencing() const { return m_Ctx->m_Objects[m_NodeId].Influence != NNull(); }

            OrbitalPhysics::Object const& GetObj() const { return m_Ctx->m_Objects[m_NodeId]; }
            OrbitalPhysics::Elements const& GetElements() const { return m_Ctx->m_Elements[m_NodeId]; }
            OrbitalPhysics::Dynamics const& GetDynamics() const { return m_Ctx->m_Dynamics[m_NodeId]; }

            LSpaceNode ParentLsp() const { return LSpaceNode{ m_Ctx->m_Tree.GetParent(m_NodeId) }; }
            ObjectNode ParentObj() const { return ObjectNode{ m_Ctx->m_Tree.GetGrandparent(m_NodeId) }; }

            LSpaceNode PrimaryLsp() const { return m_Ctx->m_LSpaces[m_Ctx->m_Tree.GetParent(m_NodeId)].Primary; }
            ObjectNode PrimaryObj() const { return m_Ctx->m_LSpaces[m_Ctx->m_Tree.GetParent(m_NodeId)].Primary.ParentObj(); }
            LSpaceNode InfluenceLsp() const { return m_Ctx->m_Objects[m_NodeId].Influence; }


            Vector3 LocalPositionFromPrimary() const
            {
                return m_Ctx->m_Objects[m_NodeId].State.Position +
                    LSpaceNode(m_Ctx->m_Tree[m_NodeId].Parent).LocalOffsetFromPrimary();
            }

            size_t GetLocalSpaces(::std::vector<LSpaceNode>& lspNodes) const
            {
                size_t numChildren = 0;
                TNodeId first = m_Ctx->m_Tree[m_NodeId].FirstChild;
                if (first != OrbitalPhysics::NNull) {
                    TNodeId child = first;
                    do {
                        numChildren++;
                        lspNodes.emplace_back(child);
                        child = m_Ctx->m_Tree[child].NextSibling;
                        LV_CORE_ASSERT(child != OrbitalPhysics::NNull, "Sibling linked-list is broken!");
                    } while (child != first);
                }
                return numChildren;
            }

            void SetLocalSpace(LSpaceNode newLspNode) const
            {
                LV_ASSERT(!IsRoot() && !IsNull() && !newLspNode.IsNull(), "Invalid nodes!");

                m_Ctx->m_Tree.Move(m_NodeId, newLspNode.Id());

                ComputeStateValidity(*this);
                TryComputeAttributes(*this);
                SubtreeCascadeAttributeChanges(*this);
            }

            void SetMass(double mass) const
            {
                m_Ctx->m_Objects[m_NodeId].State.Mass = mass;
                ComputeStateValidity(*this);
                TryComputeAttributes(*this); /* NOTE: this should be redundant as orbital motion is independent of orbiter mass, but do it anyway just for consistency */
                SubtreeCascadeAttributeChanges(*this);
            }

            void SetPosition(Vector3 const& position) const
            {
                LV_ASSERT(!IsNull() && !IsRoot(), "Cannot set position of root or null object!");

                m_Ctx->m_Objects[m_NodeId].State.Position = position;
                ComputeStateValidity(*this);
                TryComputeAttributes(*this);
                SubtreeCascadeAttributeChanges(*this);
            }

            void SetVelocity(Vector3d const& velocity) const
            {
                LV_ASSERT(!IsNull() && !IsRoot(), "Cannot set position of root or null object!");

                m_Ctx->m_Objects[m_NodeId].State.Velocity = velocity;
                TryComputeAttributes(*this);
                SubtreeCascadeAttributeChanges(*this);
            }

            /// <summary>
            /// Returns velocity for a circular counter-clockwise orbit around the object's current primary, given its current mass and position.
            /// </summary>
            /// <param name="object">Physics object ID</param>
            Vector3d CircularOrbitVelocity() const
            {
                return OrbitalPhysics::CircularOrbitVelocity(
                    LSpaceNode{ m_Ctx->m_Tree[m_NodeId].Parent }, m_Ctx->m_Objects[m_NodeId].State.Position);
            }

            void SetDynamic(bool isDynamic) const
            {
                LV_ASSERT(!IsRoot(), "Cannot set root object dynamics!");

                if (isDynamic) {
                    m_Ctx->m_Dynamics.GetOrAdd(m_NodeId);
                }
                else {
                    m_Ctx->m_Dynamics.TryRemove(m_NodeId);
                }
                ComputeStateValidity(*this);
                TryComputeAttributes(*this);
            }
        private:
            operator TNodeId() const { return m_NodeId; }
        public:
            bool operator==(ObjectNode const& rhs) const { return this->m_NodeId == rhs.m_NodeId; }
            bool operator!=(ObjectNode const& rhs) const { return this->m_NodeId != rhs.m_NodeId; }
        };

        class LSpaceNode
        {
            friend class OrbitalPhysics;

            TNodeId m_NodeId;
        public:
            constexpr LSpaceNode() : m_NodeId(OrbitalPhysics::NNull) {}
            LSpaceNode(LSpaceNode const&) = default;
            LSpaceNode(TNodeId nodeId) : m_NodeId(nodeId)
            {
                if (nodeId != OrbitalPhysics::NNull) {
                    LV_CORE_ASSERT(m_Ctx->m_Tree.Has(nodeId), "Invalid ID!");
                    LV_CORE_ASSERT(m_Ctx->m_Tree.Height(nodeId) % 2 == 1, "Class is for local space nodes only!");
                    LV_CORE_ASSERT(m_Ctx->m_LSpaces.Has(nodeId), "Local space node must have a LocalSpace attribute!");
                }
            }

            TNodeId Id() const { return m_NodeId; }

            /* For OrbitalPhysics/internal use*/
        private:
            Node const& Node() const { return m_Ctx->m_Tree[m_NodeId]; }
            int Height() const { return m_Ctx->m_Tree.Height(m_NodeId); }
            LocalSpace& LSpace() const { return m_Ctx->m_LSpaces[m_NodeId]; }

            /* For user application/external use */
        public:
            static constexpr LSpaceNode NNull() { return {}; }
            bool IsNull() const { return m_NodeId == OrbitalPhysics::NNull; }
            bool IsRoot() const { return m_NodeId == kRootLspId; }
            bool IsHighestLSpaceOnObject() const { return m_NodeId == m_Ctx->m_Tree[m_Ctx->m_Tree.GetParent(m_NodeId)].FirstChild; }
            bool Influencing() const { return m_Ctx->m_LSpaces[m_NodeId].Influencing; }

            LocalSpace const& GetLSpace() const { return m_Ctx->m_LSpaces[m_NodeId]; }

            ObjectNode ParentObj() const { return ObjectNode{ m_Ctx->m_Tree.GetParent(m_NodeId) }; }
            LSpaceNode ParentLsp() const { return LSpaceNode{ m_Ctx->m_Tree.GetGrandparent(m_NodeId) }; }

            LSpaceNode PrimaryLsp() const { return m_Ctx->m_LSpaces[m_NodeId].Primary; }
            ObjectNode PrimaryObj() const { return m_Ctx->m_LSpaces[m_NodeId].Primary.ParentObj(); }

            size_t GetLocalObjects(std::vector<ObjectNode>& objNodes) const
            {
                size_t numChildren = 0;
                TNodeId first = m_Ctx->m_Tree[m_NodeId].FirstChild;
                if (first != OrbitalPhysics::NNull) {
                    TNodeId child = first;
                    do {
                        numChildren++;
                        objNodes.emplace_back(child);
                        child = m_Ctx->m_Tree[child].NextSibling;
                        LV_CORE_ASSERT(child != OrbitalPhysics::NNull, "Sibling linked-list is broken!");
                    } while (child != first);
                }
                return numChildren;
            }

            LSpaceNode NextHigherLSpace() const
            {
                return LSpaceNode{ m_Ctx->m_Tree[m_NodeId].PrevSibling == OrbitalPhysics::NNull
                    ? m_Ctx->m_Tree.GetGrandparent(m_NodeId)
                    : m_Ctx->m_Tree[m_NodeId].PrevSibling };
            }

            Vector3 LocalOffsetFromPrimary() const
            {
                return LocalOffsetFromPrimary(m_NodeId, m_Ctx->m_LSpaces[m_NodeId].Primary.Id());
            }

            void SetRadius(float radius) const
            {
                LV_CORE_ASSERT(!m_Ctx->m_LSpaces[m_NodeId].Influencing, "Cannot set radius of influencing local spaces!");
                LV_CORE_ASSERT(m_NodeId != kRootLspId, "Cannot set radius of root local space! (See OrbitalPhysics::SetRootSpaceScaling())");
                LV_CORE_ASSERT(radius < kMaxLSpaceRadius + kEpsLSpaceRadius &&
                    radius > kMinLSpaceRadius - kEpsLSpaceRadius, "Attempted to set invalid radius!");

                m_Ctx->m_LSpaces[m_NodeId].Radius = radius;
                m_Ctx->m_LSpaces[m_NodeId].MetersPerRadius = (double)radius * (Height() == 1
                    ? GetRootLSpaceNode().LSpace().MetersPerRadius
                    : m_Ctx->m_LSpaces[m_Ctx->m_Tree.GetGrandparent(m_NodeId)].MetersPerRadius);
                SubtreeCascadeAttributeChanges(m_NodeId);
            }

            /// <summary>
            /// Sets local space radius of object to given radius if the change is valid.
            /// </summary>
            /// <returns>True if successfully changed, false otherwise</returns>
            bool TrySetRadius(float radius) const
            {
                if (!m_Ctx->m_LSpaces[m_NodeId].Influencing &&
                    radius < kMaxLSpaceRadius + kEpsLSpaceRadius &&
                    radius > kMinLSpaceRadius - kEpsLSpaceRadius)
                {
                    SetRadius(radius);
                    return true;
                }
                LV_CORE_ASSERT(!m_Ctx->m_LSpaces[m_NodeId].Influencing, "Local-space radius of influencing entities cannot be manually set (must be set equal to radius of influence)!");
                LV_CORE_WARN("Attempted to set invalid local-space radius ({0}): must be in the range [{1}, {2}]", radius, kMinLSpaceRadius, kMaxLSpaceRadius);
                return false;
            }
        private:
            operator TNodeId() const { return m_NodeId; }
        public:
            bool operator==(ObjectNode const& rhs) const { return this->m_NodeId == rhs.m_NodeId; }
            bool operator!=(ObjectNode const& rhs) const { return this->m_NodeId != rhs.m_NodeId; }
        private:
            Vector3 LocalOffsetFromPrimary(TNodeId lspId, TNodeId primaryLspId) const
            {
                LV_CORE_ASSERT(m_Ctx->m_Tree.Height(lspId) % 2 == 1 && m_Ctx->m_Tree.Height(primaryLspId) % 2 == 1, "Invalid IDs!");
                if (lspId == primaryLspId) return Vector3::Zero();

                TNodeId lspParentObjId = m_Ctx->m_Tree[lspId].Parent;
                return (m_Ctx->m_Objects[lspParentObjId].State.Position +
                    LocalOffsetFromPrimary(m_Ctx->m_Tree[lspParentObjId].Parent, primaryLspId))
                    / m_Ctx->m_LSpaces[lspId].Radius;
            }
        };

        /*** Object enums ***/
    public:
        enum class Validity
        {
            InvalidParent = 0,
            InvalidMass = 1,
            InvalidPosition = 2,
            InvalidPath = 3,
            Valid = 100
        };

        enum class OrbitType
        {
            Circle = 0,
            Ellipse = 1,
            Hyperbola = 2
        };

        /*** Object members ***/
    public:
        struct State
        {
            /* Required attribute - all physics objects are expected to have a physics state */
            double Mass = 0.0;
            Vector3 Position = { 0.f };
            Vector3d Velocity = { 0.0 };
            Vector3d Acceleration = { 0.0 };
        };

    private:
        struct Integration
        {
            enum class Method {
                Angular = 0,
                Linear = 1
            };

            double PrevDT = 0.0;
            double UpdateTimer = 0.0;
            float DeltaTrueAnomaly = 0.f;

            ObjectNode UpdateNext = {};
            Method Method = Method::Angular; /* set to Angular by default in TryComputeAttributes() anyway */
        };

        /*** Attributes ***/
    public:
        struct Object
        {
            Validity Validity = Validity::InvalidParent;
            State State;
            LSpaceNode Influence = {}; /* Local space node representing this object's sphere of influence: Null if object is not influencing */

        private:
            friend class OrbitalPhysics;

            Integration Integration;
        };

        class LocalSpace
        {
        public:
            float Radius = 0.f; /* Measured in parent's influence */
            double MetersPerRadius = 0.f;

            LSpaceNode Primary = {};
            bool Influencing = false; /* True if the parent object is the local dominant source of gravity, i.e, this LSP is less than or equal to the parent's influence LSP */
        };

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

        /*** Node helpers ***/
    private:
        static ObjectNode NewObjectNode(LSpaceNode parentNode)
        {
            TNodeId newNodeId = m_Ctx->m_Tree.New(parentNode.Id());
            m_Ctx->m_Objects.Add(newNodeId);
            m_Ctx->m_Elements.Add(newNodeId);
            return ObjectNode{ newNodeId };
        }

        static void RemoveObjectNode(ObjectNode objNode)
        {
            m_Ctx->m_Objects.Remove(objNode.Id());
            m_Ctx->m_Elements.Remove(objNode.Id());
            m_Ctx->m_Dynamics.TryRemove(objNode.Id());
            m_Ctx->m_Tree.Remove(objNode.Id());
        }

        /// <summary>
        /// Moves object from its current local space to the next higher local space, recomputing relative position to preserve absolute position.
        /// </summary>
        static void PromoteObjectNode(ObjectNode objNode)
        {
            LSpaceNode oldLspNode = objNode.ParentLsp();
            LSpaceNode newLspNode = oldLspNode.NextHigherLSpace();

            Object& obj = objNode.Object();
            float rescalingFactor = 1.f;
            if (oldLspNode.IsHighestLSpaceOnObject()) {
                rescalingFactor = oldLspNode.LSpace().Radius;
                obj.State.Position = (obj.State.Position * rescalingFactor) + oldLspNode.ParentObj().Object().State.Position;
                obj.State.Velocity = (obj.State.Velocity * (double)rescalingFactor) + oldLspNode.ParentObj().Object().State.Velocity;
            }
            else {
                rescalingFactor = oldLspNode.LSpace().Radius / newLspNode.LSpace().Radius;
                obj.State.Position = obj.State.Position * rescalingFactor;
                obj.State.Velocity = obj.State.Velocity * (double)rescalingFactor;
            }

            // TODO : rescale non-influencing local spaces ?
            // 
            //std::vector<LSpaceNode> localSpaces = {};
            //size_t numLsps = m_UpdateQueueFront.GetLocalSpaces(localSpaces);
            //for (size_t i = 0; i < numLsps; i++) {
            //    if (localSpaces[i].LSpace().Influencing) break;
            //    localSpaces[i].LSpace().Radius *= rescalingFactor; /* preserves absolute radius of local space */
            //}

            m_Ctx->m_Tree.Move(objNode.Id(), newLspNode.Id());

            ComputeStateValidity(objNode);
            TryComputeAttributes(objNode);
            SubtreeCascadeAttributeChanges(objNode.Id());
        }

        static LSpaceNode NewLSpaceNode(ObjectNode parentNode)
        {
            TNodeId newNodeId = m_Ctx->m_Tree.New(parentNode.Id());
            m_Ctx->m_LSpaces.Add(newNodeId);
            return LSpaceNode{ newNodeId };
        }

        static void RemoveLSpaceNode(LSpaceNode lspNode)
        {
            std::vector<ObjectNode> localObjs;
            for (size_t i = 0; i < lspNode.GetLocalObjects(localObjs); i++) {
                PromoteObjectNode(localObjs[i]);
            }
            m_Ctx->m_LSpaces.Remove(lspNode.Id());
            m_Ctx->m_Tree.Remove(lspNode.Id());
        }

        /*** Simulation resources ***/
    public:
        class Context
        {
            friend class OrbitalPhysics;

            Tree m_Tree;

            AttributeStorage<Object> m_Objects;
            AttributeStorage<LocalSpace> m_LSpaces;
            AttributeStorage<Elements> m_Elements;
            AttributeStorage<Dynamics> m_Dynamics;

            ObjectNode m_UpdateQueueFront = {};
        public:
            Context()
            {
                m_Tree.New(); /* kRootObjId (0) */
                m_Tree.New(); /* kRootLspId (1) */
                LV_CORE_ASSERT(m_Tree.Has(kRootObjId), "Context failed to create root object node!");
                LV_CORE_ASSERT(m_Tree.Has(kRootLspId), "Context failed to create root local space node!");

                auto& rootObj = m_Objects.Add(kRootObjId);
                rootObj.Validity = Validity::InvalidMass; /* Object::Validity is by default initialised to InvalidParent, but that is meaningless for the root object (which cannot be parented) */

                auto& rootLsp = m_LSpaces.Add(kRootLspId);
                rootLsp.Radius = 1.f;
                rootLsp.Influencing = true;
            }
            Context(Context const&) = default;
            ~Context()
            {
                // Estimating maximum object allocation for optimising vectors -> arrays
                LV_CORE_INFO("OrbitalPhysics final tree size: {0} ({1} objects, {2} local spaces)",
                    m_Tree.Size(), m_Objects.Size(), m_LSpaces.Size());
            }

        public:
            std::function<void(ObjectNode)> m_LSpaceChangedCallback;
        };

        static void SetContext(Context* ctx) { m_Ctx = ctx; }
    private:
        static Context* m_Ctx;

        static constexpr TNodeId kRootObjId = 0;
        static constexpr TNodeId kRootLspId = 1;

        /*** Simulation helpers ***/
    private:
        static bool ValidPosition(ObjectNode objNode)
        {
            static constexpr float kEscapeDistanceSqr = kLocalSpaceEscapeRadius * kLocalSpaceEscapeRadius;

            return objNode.Object().State.Position.SqrMagnitude() < kEscapeDistanceSqr;
                /* TODO - check for influence overlaps */;
        }

        static bool ValidMass(ObjectNode objNode)
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

        static bool ValidParent(ObjectNode objNode)
        {
            if (objNode.IsRoot()) return true;
            if (LSpaceNode(kRootLspId).LSpace().MetersPerRadius > 0.0) {
                return objNode.ParentObj().Object().Validity == Validity::Valid;
            }
            LV_WARN("OrbitalPhysics root scaling has not been set!");
            return false;
        }

        static bool ComputeStateValidity(ObjectNode objNode)
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

        inline static float OrbitEquation(ObjectNode objNode, float trueAnomaly)
        {
            return objNode.Elements().P / (1.f + objNode.Elements().E * cosf(trueAnomaly));
        }

        static Vector3 ObjectPositionAtTrueAnomaly(ObjectNode objNode, float trueAnomaly)
        {
            float radiusAtTrueAnomaly = OrbitEquation(objNode, trueAnomaly);
            Vector3 directionAtTrueAnomaly = cosf(trueAnomaly) * objNode.Elements().PerifocalX
                + sinf(trueAnomaly) * objNode.Elements().PerifocalY;
            Vector3 positionFromPrimary = radiusAtTrueAnomaly * directionAtTrueAnomaly;
            return positionFromPrimary - objNode.ParentLsp().LocalOffsetFromPrimary();
        }

        static void ComputeInfluence(ObjectNode objNode)
        {
            LV_CORE_ASSERT(!objNode.IsRoot(), "Cannot compute influence of root object!");

            Object& obj = objNode.Object();

            /* Radius of influence = a(m / M)^0.4
             * Semi-major axis must be in the order of 1,
             * so the order of ROI is determined by (m / M)^0.4 */
            static constexpr double kMinimumMassFactor = 1e-4; // TODO : test for optimal values
            double massFactor = pow(obj.State.Mass / objNode.PrimaryObj().Object().State.Mass, 0.4);
            if (massFactor > kMinimumMassFactor) {
                LSpaceNode lspNode = obj.Influence.IsNull() ? NewLSpaceNode(objNode) : (LSpaceNode)obj.Influence;
                obj.Influence = lspNode;

                LocalSpace& lsp = lspNode.LSpace();
                lsp.Influencing = true;
                lsp.Primary = lspNode; /* an influencing local space must be its own primary */

                lsp.Radius = objNode.Elements().SemiMajor * massFactor;
                lsp.MetersPerRadius = objNode.ParentLsp().LSpace().MetersPerRadius * (double)lsp.Radius;
                SubtreeCascadeAttributeChanges(lspNode.Id());

                // TODO : handle non-influencing sibling LSpaces!
            }
            else if (!obj.Influence.IsNull()) {
                // Object was previously influencing
                RemoveLSpaceNode(obj.Influence);

                // TODO : handle sibling LSpaces!

                /* LSpaceNode subInflNode = inflNode;
                if (inflNode.Node().FirstChild == NNull) {
                    subInflNode = LSpaceNode{ inflNode.Node().NextSibling };
                    RemoveLSpaceNode(inflNode);
                }
                while (!subInflNode.IsNull()) {
                    subInflNode.LSpace().Influencing = false;
                    subInflNode.Primary = objNode.PrimaryLsp();
                    SubtreeCascadeAttributeChanges(subInflNode.Id());
                    subInflNode = LSpaceNode{ subInflNode.Node().NextSibling };
                }*/

                obj.Influence = LSpaceNode::NNull();
            }
        }

        static void ComputeDynamics(ObjectNode objNode)
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
                if (escapesLocalSpace && objNode.ParentLsp().IsRoot()) {
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

        static void ComputeElements(ObjectNode objNode)
        {
            LV_CORE_ASSERT(!objNode.IsRoot(), "Cannot compute elements on root object!");

            auto& obj = objNode.Object();
            auto& elems = objNode.Elements();

            auto lspNode = objNode.ParentLsp();
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


        inline static double ComputeObjDT(double velocityMagnitude, double minDT = kDefaultMinDT)
        {
            if (velocityMagnitude > 0) {
                return std::max(kMinUpdateDistanced / velocityMagnitude, minDT);
            }
            return minDT;
        }


        static void TryComputeAttributes(ObjectNode objNode)
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
        /// Returns speed for a circular orbit around the local primary (not circular in local space if local space is not influencing) at the given distance from the primary (measured in local space radii).
        /// Assumes orbiter has insignificant mass compared to primary.
        /// </summary>
        static double CircularOrbitSpeed(LSpaceNode lspNode, float localRadius)
        {
            /* ||V_circular|| = sqrt(mu / ||r||), where mu is the gravitational parameter of the orbit */
            return sqrt(kGravitational * lspNode.PrimaryObj().Object().State.Mass * pow(lspNode.LSpace().MetersPerRadius, -3.0) / (double)localRadius);
        }


        /// <summary>
        /// Returns velocity for a circular counter-clockwise orbit around the given primary at the given position.
        /// Assumes orbiter has insignificant mass compared to primary.
        /// </summary>
        /// <param name="object">Physics object ID</param>
        static Vector3d CircularOrbitVelocity(LSpaceNode lspNode, Vector3 const& localPosition)
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


        static void UpdateQueuePushFront(ObjectNode objNode)
        {
            if (m_Ctx->m_UpdateQueueFront.IsNull()) {
                m_Ctx->m_UpdateQueueFront = objNode;
                objNode.Object().Integration.UpdateNext = ObjectNode::NNull();
            }
            else {
                objNode.Object().Integration.UpdateNext = m_Ctx->m_UpdateQueueFront;
                m_Ctx->m_UpdateQueueFront = objNode;
            }
        }

        /// <summary>
        /// Removes the given object from the update queue.
        /// Attempting to remove an object which is not in the queue will result in an array out-of-bounds error caused by executing 'm_Objects[Null]'.
        /// See also: UpdateQueueSafeRemove().
        /// </summary>
        static void UpdateQueueRemove(ObjectNode objNode)
        {
            LV_CORE_ASSERT(!m_Ctx->m_UpdateQueueFront.IsNull(), "Attempting to remove item from empty queue!");
            if (m_Ctx->m_UpdateQueueFront == objNode) {
                m_Ctx->m_UpdateQueueFront = objNode.Object().Integration.UpdateNext;
                objNode.Object().Integration.UpdateNext = ObjectNode::NNull();
                return;
            }
            ObjectNode queueItem = m_Ctx->m_UpdateQueueFront,
                queueNext = m_Ctx->m_UpdateQueueFront.Object().Integration.UpdateNext;
            while (queueNext != objNode) {
                LV_CORE_ASSERT(!queueNext.IsNull(), "UpdateQueueRemove() could not find the given object in the update queue!");
                queueItem = queueNext;
                queueNext = queueNext.Object().Integration.UpdateNext;
            }
            queueItem.Object().Integration.UpdateNext = objNode.Object().Integration.UpdateNext;
            objNode.Object().Integration.UpdateNext = ObjectNode::NNull();
        }

        /// <summary>
        /// Removes the given object from the update queue, if it exists in the update queue.
        /// </summary>
        /// <returns>True if object was found and removed, false otherwise.</returns>
        static bool UpdateQueueSafeRemove(ObjectNode objNode)
        {
            if (m_Ctx->m_UpdateQueueFront.IsNull()) return false;
            if (m_Ctx->m_UpdateQueueFront == objNode) {
                m_Ctx->m_UpdateQueueFront = objNode.Object().Integration.UpdateNext;
                objNode.Object().Integration.UpdateNext = ObjectNode::NNull();
                return true;
            }
            ObjectNode queueItem = m_Ctx->m_UpdateQueueFront,
                queueNext = m_Ctx->m_UpdateQueueFront.Object().Integration.UpdateNext;
            while (!queueNext.IsNull()) {
                if (queueNext == objNode) {
                    queueItem.Object().Integration.UpdateNext = objNode.Object().Integration.UpdateNext;
                    objNode.Object().Integration.UpdateNext = ObjectNode::NNull();
                    return true;
                }
                queueItem = queueNext;
                queueNext = queueNext.Object().Integration.UpdateNext;
            }
            return false;
        }

        /// <summary>
        /// Assumes the first entry in the queue is the only entry which is potentially unsorted.
        /// </summary>
        static void UpdateQueueSortFront()
        {
            LV_CORE_ASSERT(!m_Ctx->m_UpdateQueueFront.IsNull(), "Attempting to sort empty queue!");

            ObjectNode objNode = m_Ctx->m_UpdateQueueFront;
            auto& integ = objNode.Object().Integration;

            ObjectNode queueItem = integ.UpdateNext;
            if (queueItem.IsNull()) return;
            if (integ.UpdateTimer < queueItem.Object().Integration.UpdateTimer) return;
            m_Ctx->m_UpdateQueueFront = queueItem;

            ObjectNode queueNext = queueItem.Object().Integration.UpdateNext;
            while (!queueNext.IsNull()) {
                if (integ.UpdateTimer < queueNext.Object().Integration.UpdateTimer) break;
                queueItem = queueNext;
                queueNext = queueNext.Object().Integration.UpdateNext;
            }
            queueItem.Object().Integration.UpdateNext = objNode;
            integ.UpdateNext = queueNext;
        }


        static void SubtreeCascadeAttributeChanges(TNodeId rootNodeId)
        {
            std::vector<TNodeId> tree{};
            m_Ctx->m_Tree.GetSubtree(rootNodeId, tree);
            for (auto nodeId : tree) {
                if (IsLocalSpace(nodeId)) continue;

                // TODO : preserve orbit shapes ?
                ObjectNode subObjNode{ nodeId };
                ComputeStateValidity(subObjNode);
                TryComputeAttributes(subObjNode);
            }
        }

        /*** Usage ***/
    public:
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


        static void OnUpdate(Timestep dT)
        {
#ifdef LV_DEBUG // debug pre-update
            /*std::chrono::steady_clock::time_point updateStart = std::chrono::steady_clock::now();
            static std::vector<std::chrono::steady_clock::time_point> timesOfLastPeriapsePassage = { };
            timesOfLastPeriapsePassage.resize(m_Objects.size(), std::chrono::steady_clock::time_point::min());
            for (ObjStats& stats : m_Stats.ObjStats) {
                stats.NumObjectUpdates = 0;
            }
            m_Stats.ObjStats.resize(m_Objects.size(), ObjStats());*/
#endif

            double minObjDT = dT / kMaxObjectUpdates;

            // Update all objects with timers less than 0
            while (m_Ctx->m_UpdateQueueFront.Object().Integration.UpdateTimer < 0.0)
            {
                auto& obj = m_Ctx->m_UpdateQueueFront.Object();
                auto& elems = m_Ctx->m_UpdateQueueFront.Elements();
                bool isDynamic = m_Ctx->m_UpdateQueueFront.IsDynamic();

#ifdef LV_DEBUG // debug object pre-update
                /*m_Stats.ObjStats[m_UpdateQueueFront].NumObjectUpdates += 1;
                float prevTrueAnomaly = elems.TrueAnomaly;*/
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
                        Vector3 positionFromPrimary = m_Ctx->m_UpdateQueueFront.LocalPositionFromPrimary();
                        float posMag2 = positionFromPrimary.SqrMagnitude();
                        Vector3 posDir = positionFromPrimary / sqrtf(posMag2);
                        obj.State.Acceleration = -(Vector3d)posDir * elems.Grav / (double)posMag2;
                        if (isDynamic) {
                            obj.State.Acceleration += m_Ctx->m_UpdateQueueFront.Dynamics().ContAcceleration;
                        }
                        obj.Integration.Method = Integration::Method::Linear;
                        LV_CORE_TRACE("Object {0} switched from angular to linear integration!", m_Ctx->m_UpdateQueueFront.m_NodeId);
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
                        obj.State.Position = positionFromPrimary - m_Ctx->m_UpdateQueueFront.ParentLsp().LocalOffsetFromPrimary();
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
                    Vector3 positionFromPrimary = m_Ctx->m_UpdateQueueFront.LocalPositionFromPrimary();
                    float r2 = positionFromPrimary.SqrMagnitude();
                    Vector3d newAcceleration = -(Vector3d)positionFromPrimary * elems.Grav / (double)(r2 * sqrtf(r2));
                    bool isDynamicallyAccelerating = false;
                    if (isDynamic) {
                        newAcceleration += m_Ctx->m_UpdateQueueFront.Dynamics().ContAcceleration;
                        isDynamicallyAccelerating = !m_Ctx->m_UpdateQueueFront.Dynamics().ContAcceleration.IsZero();
                    }
                    obj.State.Velocity += 0.5 * (obj.State.Acceleration + newAcceleration) * objDT;
                    obj.State.Acceleration = newAcceleration;

                    if (isDynamicallyAccelerating) {
                        ComputeElements(m_Ctx->m_UpdateQueueFront);
                        ComputeDynamics(m_Ctx->m_UpdateQueueFront);
                        ComputeInfluence(m_Ctx->m_UpdateQueueFront);
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
                /*if (elems.TrueAnomaly < prevTrueAnomaly) {
                    auto timeOfPeriapsePassage = std::chrono::steady_clock::now();
                    if (timesOfLastPeriapsePassage[m_UpdateQueueFront] != std::chrono::steady_clock::time_point::min()) {
                        m_Stats.ObjStats[m_UpdateQueueFront].LastOrbitDuration = timeOfPeriapsePassage - timesOfLastPeriapsePassage[m_UpdateQueueFront];
                        m_Stats.ObjStats[m_UpdateQueueFront].LastOrbitDurationError = (elems.T - m_Stats.ObjStats[m_UpdateQueueFront].LastOrbitDuration.count()) / elems.T;
                    }
                    timesOfLastPeriapsePassage[m_UpdateQueueFront] = timeOfPeriapsePassage;
                }*/
#endif

                // Test for orbit events
                if (isDynamic) {
                    auto& dynamics = m_Ctx->m_UpdateQueueFront.Dynamics();

                    float escapeTrueAnomaly = dynamics.EscapeTrueAnomaly;
                    if (escapeTrueAnomaly > 0.f && elems.TrueAnomaly < PIf && elems.TrueAnomaly > escapeTrueAnomaly) {
                        LV_CORE_ASSERT(sqrtf(obj.State.Position.SqrMagnitude()) > kLocalSpaceEscapeRadius, "False positive on escape test!");
                        LV_CORE_ASSERT(!m_Ctx->m_UpdateQueueFront.ParentLsp().IsRoot(), "Cannot escape root local space!");

                        PromoteObjectNode(m_Ctx->m_UpdateQueueFront);

                        if (m_Ctx->m_LSpaceChangedCallback) {
                            m_Ctx->m_LSpaceChangedCallback(m_Ctx->m_UpdateQueueFront);
                        }
                        else {
                            LV_WARN("Callback function 'LSpaceChangedCallback' is not set in this context!");
                        }

                        LV_CORE_ASSERT(obj.Validity == Validity::Valid, "Invalid dynamics after escape!");

                        objDT = ComputeObjDT(sqrt(obj.State.Velocity.SqrMagnitude()), minObjDT);
                        Vector3 positionFromPrimary = m_Ctx->m_UpdateQueueFront.LocalPositionFromPrimary();
                        float posMag2 = positionFromPrimary.SqrMagnitude();
                        obj.Integration.DeltaTrueAnomaly = (float)(objDT * elems.H) / posMag2;

                        // TODO : handle cases where dynamic acceleration is non-zero, e.g, bool isDynamicallyAccelerating = m_Dynamics.Has(object) && !m_Dynamics[object].ContAcceleration.IsZero()
                        if (obj.Integration.DeltaTrueAnomaly > kMinUpdateTrueAnomaly) {
                            LV_CORE_ASSERT(dynamics.ContAcceleration.IsZero(), "Dynamic acceleration not handled!");
                            obj.Integration.Method = Integration::Method::Angular;
                        }
                        else {
                            Vector3 posDir = positionFromPrimary / sqrtf(posMag2);
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
            ObjectNode objNode = m_Ctx->m_UpdateQueueFront;
            do { 
                objNode.Object().Integration.UpdateTimer -= dT;
                objNode = objNode.Object().Integration.UpdateNext;
            } while (!objNode.IsNull());

#ifdef LV_DEBUG // debug post-update
            //m_Stats.UpdateTime = std::chrono::steady_clock::now() - updateStart;
#endif
        }


        static ObjectNode GetRootObjectNode()
        {
            return { kRootObjId };
        }

        static LSpaceNode GetRootLSpaceNode()
        {
            return { kRootLspId };
        }

        /// <summary>
        /// Sets scaling of the root local space.
        /// Scaling is measured in meters per unit-radius of the root local space.
        /// E.g, a distance with length 1 in the root orbital space has a simulated length equal to the root scaling.
        /// </summary>
        /// <param name="meters"></param>
        static void SetRootSpaceScaling(double meters)
        {
            LSpaceNode(kRootLspId).LSpace().MetersPerRadius = meters;
            SubtreeCascadeAttributeChanges(kRootLspId);
        }


        /// <summary>
        /// Checks if the given ID identifies an existing physics object.
        /// </summary>
        /// <param name="object">ID of physics object in question</param>
        /// <returns>True if ID is that of a physics object which has been created and not yet destroyed, false otherwise.</returns>
        static bool Has(TNodeId nodeId)
        {
            return m_Ctx->m_Tree.Has(nodeId);
        }


        /// <summary>
        /// Create an orbital physics object in the specified orbital space.
        /// </summary>
        /// <param name="userId">ID of the user-object to be associated with the created physics object</param>
        /// <returns>ID of the created physics object</returns>
        static ObjectNode Create(LSpaceNode lspNode, double mass, Vector3 const& position, Vector3d const& velocity, bool dynamic = false)
        {
            LV_CORE_ASSERT(!lspNode.IsNull(), "Invalid local space!");

            ObjectNode newObjNode = NewObjectNode(lspNode);
            auto& obj = newObjNode.Object();
            obj.State.Mass = mass;
            obj.State.Position = position;
            obj.State.Velocity = velocity;

            if (dynamic) {
                m_Ctx->m_Dynamics.Add(newObjNode.Id());
            }

            ComputeStateValidity(newObjNode);
            TryComputeAttributes(newObjNode);

            return newObjNode;
        }

        /// <summary>
        /// Create an orbital physics object in the specified orbital space.
        /// New object's velocity defaults to that of a circular orbit.
        /// </summary>
        /// <param name="userId">ID of the user-object to be associated with the created physics object</param>
        /// <returns>ID of the created physics object</returns>
        static ObjectNode Create(LSpaceNode lspNode, bool mass, Vector3 const& position, bool dynamic = false)
        {
            LV_CORE_ASSERT(!lspNode.IsNull(), "Invalid local space!");

            return Create(lspNode, mass, position, CircularOrbitVelocity(lspNode, position), dynamic);
        }

        /// <summary>
        /// Create an uninitialised orbital physics object in the specified orbital space.
        /// </summary>
        /// <param name="userId">ID of the user-object to be associated with the created physics object</param>
        /// <returns>ID of the created physics object</returns>
        static ObjectNode Create(LSpaceNode lspNode, bool dynamic = false)
        {
            LV_CORE_ASSERT(!lspNode.IsNull(), "Invalid local space!");

            return Create(lspNode, 0.0, { 0.f }, { 0.0 }, dynamic);
        }

        /// <summary>
        /// Create an uninitialised orbital physics object in the root orbital space.
        /// </summary>
        /// <param name="userId">ID of the user-object to be associated with the created physics object</param>
        /// <returns>ID of the created physics object</returns>
        static ObjectNode Create(bool dynamic = false)
        {
            return Create({kRootLspId}, 0.0, {0.f}, {0.0}, dynamic);
        }

        /// <summary>
        /// Destroy an orbital physics object.
        /// Children are re-parented to the object's parent.
        /// </summary>
        /// <param name="objectId">ID of the physics object to be destroyed</param>
        static void Destroy(ObjectNode objNode)
        {
            LV_CORE_ASSERT(!objNode.IsNull(), "Invalid node!");

            // Move children into parent local space
            LSpaceNode parentLsp = objNode.ParentLsp();
            Object& obj = objNode.Object();
            std::vector<LSpaceNode> lspaces;
            for (size_t i = 0; i < objNode.GetLocalSpaces(lspaces); i++)
            {
                float rescalingFactor = lspaces[i].LSpace().Radius;
                std::vector<ObjectNode> localObjs;
                for (size_t j = 0; j < lspaces[i].GetLocalObjects(localObjs); j++)
                {
                    Object& subObj = localObjs[j].Object();
                    subObj.State.Position = (subObj.State.Position * rescalingFactor) + obj.State.Position;
                    subObj.State.Velocity = (subObj.State.Velocity * (double)rescalingFactor) + obj.State.Velocity;

                    m_Ctx->m_Tree.Move(localObjs[j].Id(), parentLsp.Id());

                    ComputeStateValidity(localObjs[j]);
                    TryComputeAttributes(localObjs[j]);
                    SubtreeCascadeAttributeChanges(localObjs[j].Id());
                }
            }

            RemoveObjectNode(objNode);
        }


        static LSpaceNode AddLocalSpace(ObjectNode objNode, float radius = kDefaultLSpaceRadius)
        {
            LSpaceNode newLspNode = NewLSpaceNode(objNode);
            newLspNode.SetRadius(radius);
            return newLspNode;
        }


        //void SetContinuousAcceleration(TObjectId object, Vector3d const& acceleration, double dT = 1.0 / 60.0)
        //{
        //    // TODO : test if calling this function before the simulation is run (e.g, from the editor in edit mode) does anything bad !!

        //    LV_CORE_ASSERT(m_Dynamics.Has(object), "Attempted to set dynamic acceleration on a non-dynamic orbiter!");

        //    auto& state = m_Objects[object].State;
        //    auto& dynamics = m_Dynamics[object];

        //    state.Position += 0.5f * (acceleration - dynamics.ContAcceleration) * powf(state.UpdateTimer, 2.f);
        //    state.Velocity += (acceleration - dynamics.ContAcceleration) * state.UpdateTimer;
        //    double newObjDT = ComputeObjDT(sqrt(state.Velocity.SqrMagnitude()));
        //    state.UpdateTimer += newObjDT - state.PrevDT; //max(kMinUpdateDistanced / state.Velocity, kDefaultMinDT) - state.PrevDT;
        //    state.PrevDT = newObjDT; // VERY UNTESTED !!!!
        //    dynamics.ContAcceleration = acceleration;

        //    LV_CORE_ASSERT(false, "Untested!");

        //    TryComputeAttributes(object);
        //}

        //void SetContinuousThrust(TObjectId object, Vector3d const& force)
        //{
        //    SetContinuousAcceleration(object, force / m_Objects[object].State.Mass);
        //}

        //void ApplyInstantAcceleration(TObjectId object, Vector3d const& acceleration)
        //{
        //    LV_CORE_ASSERT(false, "Not implemented!");
        //}

    };

}
