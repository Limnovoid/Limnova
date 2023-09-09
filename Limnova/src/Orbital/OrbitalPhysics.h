#pragma once

#include <Math/Math.h>
#include <Core/Timestep.h>


namespace Limnova
{
    class OrbitalPhysics
    {
    public:
        OrbitalPhysics() = delete;

    public:
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
        static constexpr float kMinLSpaceRadius = 0.004f;
        static constexpr float kEpsLSpaceRadius = 1e-6f;

        static constexpr float kMaxObjectUpdates = 20.f; /* highest number of updates each object is allowed before the total number of updates (across all objects) in a single frame becomes too high (resulting in unacceptable FPS drops) */
        static constexpr double kDefaultMinDT = 1.0 / (60.0 * kMaxObjectUpdates); /* above constraint expressed as a maximum delta time for a single integration step */
        static constexpr float kMaxUpdateDistance = 1e-6f; /* largest delta position we can allow before integration step becomes too visible (for when object DT is much greater than frame DT) */
        static constexpr double kMaxUpdateDistanced = (double)kMaxUpdateDistance;
        static constexpr double kMinUpdateTrueAnomaly = ::std::numeric_limits<float>::epsilon() * 1e2f; //1e-6f; /* smallest delta true anomaly we can allow before precision error becomes unacceptable for long-term angular integration */
        ////////////////////////////////////////


        /*** General-purpose array-based storage class ***/
    private:
        using TId = uint32_t;
        template<typename T>
        class Storage
        {
            std::vector<T> m_Items;
            std::unordered_set<TId> m_Empties;
        public:
            Storage() = default;
            Storage(const Storage&) = default;

            size_t Size() const
            {
                return m_Items.size() - m_Empties.size();
            }

            bool Has(TId id) const
            {
                return id < m_Items.size() && !m_Empties.contains(id);
            }

            TId New()
            {
                return GetEmpty();
            }

            T& Get(TId id)
            {
                LV_CORE_ASSERT(Has(id), "Invalid ID!");
                return m_Items[id];
            }

            T const& Get(TId id) const
            {
                LV_CORE_ASSERT(Has(id), "Invalid ID!");
                return m_Items[id];
            }

            void Erase(TId id)
            {
                LV_CORE_ASSERT(Has(id), "Invalid ID!");
                Recycle(id);
            }

            bool TryErase(TId id)
            {
                if (Has(id)) {
                    Recycle(m_Items[id]);
                    return true;
                }
                return false;
            }

            void Clear()
            {
                m_Items.clear();
                m_Empties.clear();
            }
        public:
            T& operator[](TId id)
            {
                LV_CORE_ASSERT(Has(id), "Invalid ID!");
                return m_Items[id];
            }
            T const& operator[](TId id) const
            {
                LV_CORE_ASSERT(Has(id), "Invalid ID!");
                return m_Items[id];
            }
        private:
            TId GetEmpty()
            {
                TId emptyId;
                if (m_Empties.empty()) {
                    emptyId = m_Items.size();
                    m_Items.emplace_back();
                }
                else {
                    auto it = m_Empties.begin();
                    emptyId = *it;
                    m_Empties.erase(it);
                }
                return emptyId;
            }

            void Recycle(TId id)
            {
                m_Items[id] = T();
                m_Empties.insert(id);
            }
        };


        /*** Object tree ***/
    public:
        using TNodeId = uint32_t;

        static constexpr TNodeId NNull = ::std::numeric_limits<TNodeId>::max();
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
            Storage<Node> m_Nodes;
            std::vector<int> m_Heights;
        public:
            Tree() = default;
            Tree(const Tree&) = default;

            size_t Size() const
            {
                return m_Nodes.Size();
            }

            bool Has(TNodeId nodeId) const
            {
                return m_Nodes.Has(nodeId);
            }

            TNodeId New()
            {
                TNodeId nodeId = GetEmpty();
                if (nodeId == 0) { m_Heights[nodeId] = 0; }
                else { Attach(nodeId, 0); }
                return nodeId;
            }

            TNodeId New(TNodeId parentId)
            {
                LV_CORE_ASSERT(Has(parentId), "Invalid parent ID!");

                TNodeId nodeId = GetEmpty();
                Attach(nodeId, parentId);
                return nodeId;
            }

            Node const& Get(TNodeId nodeId) const
            {
                LV_CORE_ASSERT(Has(nodeId), "Invalid node ID!");
                return m_Nodes.Get(nodeId);
            }

            int Height(TNodeId nodeId) const
            {
                LV_CORE_ASSERT(Has(nodeId), "Invalid node ID!");
                return m_Heights[nodeId];
            }

            void Remove(TNodeId nodeId)
            {
                if (nodeId == 0) { Clear(); }
                else {
                    Detach(nodeId);
                    RecycleSubtree(nodeId);
                }
            }

            void Clear()
            {
                m_Nodes.Clear();
                m_Heights.clear();
            }

            void Move(TNodeId nodeId, TNodeId newParentId)
            {
                Detach(nodeId);
                Attach(nodeId, newParentId);
            }

            void SwapWithPrevSibling(TNodeId nodeId)
            {
                LV_CORE_ASSERT(Has(nodeId), "Invalid node ID!");
                LV_CORE_ASSERT(Has(m_Nodes[nodeId].PrevSibling), "Invalid node ID!");
                auto& node = m_Nodes[nodeId];
                auto& prev = m_Nodes[node.PrevSibling];
                auto& parent = m_Nodes[node.Parent];
                if (parent.FirstChild == node.PrevSibling) { parent.FirstChild = nodeId; }
                if (prev.PrevSibling != NNull) { m_Nodes[prev.PrevSibling].NextSibling = nodeId; }
                if (node.NextSibling != NNull) { m_Nodes[node.NextSibling].PrevSibling = node.PrevSibling; }
                prev.NextSibling = node.NextSibling;
                node.NextSibling = node.PrevSibling;
                node.PrevSibling = prev.PrevSibling;
                prev.PrevSibling = nodeId;
            }

            void SwapWithNextSibling(TNodeId nodeId)
            {
                LV_CORE_ASSERT(Has(nodeId), "Invalid node ID!");
                LV_CORE_ASSERT(Has(m_Nodes[nodeId].NextSibling), "Invalid node ID!");
                auto& node = m_Nodes[nodeId];
                auto& next = m_Nodes[node.NextSibling];
                auto& parent = m_Nodes[node.Parent];
                if (parent.FirstChild == nodeId) { parent.FirstChild = node.NextSibling; }
                if (next.NextSibling != NNull) { m_Nodes[next.NextSibling].PrevSibling = nodeId; }
                if (node.PrevSibling != NNull) { m_Nodes[node.PrevSibling].NextSibling = node.NextSibling; }
                next.PrevSibling = node.PrevSibling;
                node.PrevSibling = node.NextSibling;
                node.NextSibling = next.NextSibling;
                next.NextSibling = nodeId;
            }

            size_t GetChildren(TNodeId nodeId, std::vector<TNodeId>& children) const
            {
                LV_CORE_ASSERT(Has(nodeId), "Invalid node ID!");
                size_t numChildren = 0;

                TNodeId child = m_Nodes[nodeId].FirstChild;
                while (child != NNull) {
                    numChildren++;
                    children.push_back(child);
                    child = m_Nodes[child].NextSibling;
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
                return m_Nodes[m_Nodes[nodeId].Parent].Parent;
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
                TNodeId emptyId = m_Nodes.New();
                if (emptyId >= m_Heights.size()) { m_Heights.push_back(-1); }
                return emptyId;
            }

            void RecycleSubtree(TNodeId rootId)
            {
                LV_CORE_ASSERT(Has(rootId), "Invalid root node ID!");
                TNodeId childId = m_Nodes[rootId].FirstChild;
                while (childId != NNull) {
                    childId = m_Nodes[childId].NextSibling;
                    RecycleSubtree(m_Nodes[childId].PrevSibling);
                }
                m_Nodes.Erase(rootId);
            }

            void Attach(TNodeId nodeId, TNodeId parentId)
            {
                auto& node = m_Nodes[nodeId];
                auto& parent = m_Nodes[parentId];

                // Connect to parent
                node.Parent = parentId;
                if (parent.FirstChild != NNull) {
                    // Connect to siblings
                    node.NextSibling = parent.FirstChild;
                    m_Nodes[parent.FirstChild].PrevSibling = nodeId;
                }
                parent.FirstChild = nodeId;

                m_Heights[nodeId] = m_Heights[parentId] + 1;
            }

            void Detach(TNodeId nodeId)
            {
                auto& node = m_Nodes[nodeId];
                auto& parent = m_Nodes[node.Parent];

                // Disconnect from parent
                if (parent.FirstChild == nodeId) {
                    parent.FirstChild = node.NextSibling;
                }
                node.Parent = NNull;

                // Disconnect from siblings
                if (node.NextSibling != NNull) {
                    m_Nodes[node.NextSibling].PrevSibling = node.PrevSibling;
                }
                if (node.PrevSibling != NNull) {
                    m_Nodes[node.PrevSibling].NextSibling = node.NextSibling;
                }
                node.NextSibling = node.PrevSibling = NNull;

                m_Heights[nodeId] = -1;
            }
        };

        /*** Node attribute storage class ***/
    private:
        template<typename TAttr>
        class AttributeStorage
        {
            Storage<TAttr> m_Attributes;
            std::unordered_map<TNodeId, TId> m_NodeToAttr;
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
                LV_CORE_ASSERT(!Has(nodeId), "Node already has attribute!");
                TId attrId = m_Attributes.New();
                m_NodeToAttr.insert({ nodeId, attrId });
                return m_Attributes[attrId];
            }

            TAttr& Get(TNodeId nodeId)
            {
                LV_CORE_ASSERT(Has(nodeId), "Node is missing requested attribute!");
                return m_Attributes.Get(m_NodeToAttr.at(nodeId));
            }

            TAttr& GetOrAdd(TNodeId nodeId)
            {
                return Has(nodeId) ? Get(nodeId) : Add(nodeId);
            }

            void Remove(TNodeId nodeId)
            {
                LV_CORE_ASSERT(Has(nodeId), "Node does not have the attribute to remove!");
                m_Attributes.Erase(m_NodeToAttr.at(nodeId));
                m_NodeToAttr.erase(nodeId);
            }

            bool TryRemove(TNodeId nodeId)
            {
                if (m_NodeToAttr.contains(nodeId)) {
                    m_Attributes.Erase(m_NodeToAttr.at(nodeId));
                    m_NodeToAttr.erase(nodeId);
                    return true;
                }
                return false;
            }
        public:
            TAttr& operator[](TNodeId nodeId)
            {
                LV_CORE_ASSERT(m_NodeToAttr.contains(nodeId), "Node is missing requested attribute!");
                return m_Attributes.Get(m_NodeToAttr.at(nodeId));
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

        struct Object;
        struct LocalSpace;
        struct OrbitSection;
        struct Elements;
        struct Dynamics;
        struct Integration;
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
                    LV_CORE_ASSERT(m_Ctx->m_OrbitSections.Has(m_Ctx->m_Objects.Get(nodeId).Orbit) || nodeId == kRootObjId, "Object node must have an orbit!");
                }
            }

            TNodeId Id() const { return m_NodeId; }

            /* For OrbitalPhysics/internal use*/
        private:
            Node const& Node() const { return m_Ctx->m_Tree[m_NodeId]; }
            int Height() const { return m_Ctx->m_Tree.Height(m_NodeId); }
            Object& Object() const { return m_Ctx->m_Objects[m_NodeId]; }
            OrbitSection& Orbit() const { return m_Ctx->m_OrbitSections[Object().Orbit]; }
            Dynamics& Dynamics() const { return m_Ctx->m_Dynamics[m_NodeId]; }

            /* For user application/external use */
        public:
            static constexpr ObjectNode NNull() { return {}; }
            bool IsNull() const { return m_NodeId == OrbitalPhysics::NNull; }
            bool IsRoot() const { return m_NodeId == kRootObjId; }
            bool IsDynamic() const { return m_Ctx->m_Dynamics.Has(m_NodeId); }
            bool IsInfluencing() const { return m_Ctx->m_Objects[m_NodeId].Influence != NNull(); }

            OrbitalPhysics::Object const& GetObj() const { return m_Ctx->m_Objects[m_NodeId]; }
            OrbitSection const& GetOrbit() const { return m_Ctx->m_OrbitSections[Object().Orbit]; }
            OrbitalPhysics::Dynamics const& GetDynamics() const { return m_Ctx->m_Dynamics[m_NodeId]; }

            LSpaceNode ParentLsp() const { return LSpaceNode{ m_Ctx->m_Tree.GetParent(m_NodeId) }; }
            ObjectNode ParentObj() const { return ObjectNode{ m_Ctx->m_Tree.GetGrandparent(m_NodeId) }; }

            LSpaceNode PrimaryLsp() const { return m_Ctx->m_LSpaces[m_Ctx->m_Tree.GetParent(m_NodeId)].Primary; }
            ObjectNode PrimaryObj() const { return m_Ctx->m_LSpaces[m_Ctx->m_Tree.GetParent(m_NodeId)].Primary.ParentObj(); }
            LSpaceNode SphereOfInfluence() const { return m_Ctx->m_Objects[m_NodeId].Influence; }

            Vector3 LocalPositionFromPrimary() const
            {
                return m_Ctx->m_Objects[m_NodeId].State.Position +
                    LSpaceNode(m_Ctx->m_Tree[m_NodeId].Parent).LocalOffsetFromPrimary();
            }

            Vector3d LocalVelocityFromPrimary() const
            {
                return m_Ctx->m_Objects[m_NodeId].State.Velocity +
                    LSpaceNode(m_Ctx->m_Tree[m_NodeId].Parent).LocalVelocityFromPrimary();
            }

            size_t GetLocalSpaces(::std::vector<LSpaceNode>& lspNodes) const
            {
                size_t numChildren = 0;
                TNodeId child = m_Ctx->m_Tree[m_NodeId].FirstChild;
                while (child != OrbitalPhysics::NNull) {
                    numChildren++;
                    lspNodes.emplace_back(child);
                    child = m_Ctx->m_Tree[child].NextSibling;
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
                LV_ASSERT(!IsNull(), "Cannot set mass of null object!");

                m_Ctx->m_Objects[m_NodeId].State.Mass = mass;
                ComputeStateValidity(*this);
                if (IsRoot()) {
                    auto& rootLsp = Object().Influence.LSpace();
                    rootLsp.Grav = LocalGravitationalParameter(mass, rootLsp.MetersPerRadius);
                    // TODO : exclude from release builds ?
                }
                else {
                    TryComputeAttributes(*this);
                }
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


            LSpaceNode AddLocalSpace(float radius = kDefaultLSpaceRadius)
            {
                return NewLSpaceNode(*this, radius);
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
            bool IsInfluencing() const { return m_NodeId == m_Ctx->m_LSpaces[m_NodeId].Primary.m_NodeId; } /* True if the parent object is the local dominant source of gravity, i.e, this LSP is less than or equal to the parent's influence LSP */
            bool IsSphereOfInfluence() const { return m_NodeId == ParentObj().Object().Influence.m_NodeId; } /* True if this local space represents the parent object's sphere of influence */

            LocalSpace const& GetLSpace() const { return m_Ctx->m_LSpaces[m_NodeId]; }

            ObjectNode ParentObj() const { return ObjectNode{ m_Ctx->m_Tree.GetParent(m_NodeId) }; }
            LSpaceNode ParentLsp() const { return LSpaceNode{ m_Ctx->m_Tree.GetGrandparent(m_NodeId) }; }

            LSpaceNode PrimaryLsp() const { return m_Ctx->m_LSpaces[m_NodeId].Primary; }
            ObjectNode PrimaryObj() const { return m_Ctx->m_LSpaces[m_NodeId].Primary.ParentObj(); }

            size_t GetLocalObjects(std::vector<ObjectNode>& objNodes) const
            {
                size_t numChildren = 0;
                TNodeId child = m_Ctx->m_Tree[m_NodeId].FirstChild;
                while (child != OrbitalPhysics::NNull) {
                    numChildren++;
                    objNodes.emplace_back(child);
                    child = m_Ctx->m_Tree[child].NextSibling;
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
                return LocalOffsetFromPrimary(m_NodeId, m_Ctx->m_LSpaces[m_NodeId].Primary.m_NodeId);
            }

            Vector3d LocalVelocityFromPrimary() const
            {
                return LocalVelocityFromPrimary(m_NodeId, m_Ctx->m_LSpaces[m_NodeId].Primary.m_NodeId);
            }

            void SetRadius(float radius) const
            {
                LV_CORE_ASSERT(!IsSphereOfInfluence(), "Cannot set radius of sphere of influence!");
                SetRadiusImpl(radius);
            }

            /// <summary>
            /// Sets local space radius of object to given radius if the change is valid.
            /// </summary>
            /// <returns>True if successfully changed, false otherwise</returns>
            bool TrySetRadius(float radius) const
            {
                if (!IsInfluencing() &&
                    radius < kMaxLSpaceRadius + kEpsLSpaceRadius &&
                    radius > kMinLSpaceRadius - kEpsLSpaceRadius)
                {
                    SetRadiusImpl(radius);
                    return true;
                }
                LV_CORE_ASSERT(!IsInfluencing(), "Local-space radius of influencing entities cannot be manually set (must be set equal to radius of influence)!");
                LV_CORE_WARN("Attempted to set invalid local-space radius ({0}): must be in the range [{1}, {2}]", radius, kMinLSpaceRadius, kMaxLSpaceRadius);
                return false;
            }
        private:
            operator TNodeId() const { return m_NodeId; }
        public:
            bool operator==(LSpaceNode const& rhs) const { return this->m_NodeId == rhs.m_NodeId; }
            bool operator!=(LSpaceNode const& rhs) const { return this->m_NodeId != rhs.m_NodeId; }
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

            Vector3d LocalVelocityFromPrimary(TNodeId lspId, TNodeId primaryLspId) const
            {
                LV_CORE_ASSERT(m_Ctx->m_Tree.Height(lspId) % 2 == 1 && m_Ctx->m_Tree.Height(primaryLspId) % 2 == 1, "Invalid IDs!");
                if (lspId == primaryLspId) return Vector3d::Zero();

                TNodeId lspParentObjId = m_Ctx->m_Tree[lspId].Parent;
                return (m_Ctx->m_Objects[lspParentObjId].State.Velocity +
                    LocalVelocityFromPrimary(m_Ctx->m_Tree[lspParentObjId].Parent, primaryLspId))
                    / m_Ctx->m_LSpaces[lspId].Radius;
            }

            /// <summary>
            /// Internal function allows setting radius on sphere of influence
            /// </summary>
            void SetRadiusImpl(float radius) const
            {
                auto& node = m_Ctx->m_Tree[m_NodeId];
                auto& lsp = m_Ctx->m_LSpaces[m_NodeId];

                LV_CORE_ASSERT(m_NodeId != kRootLspId, "Cannot set radius of root local space! (See OrbitalPhysics::SetRootSpaceScaling())");
                LV_CORE_ASSERT(radius < kMaxLSpaceRadius + kEpsLSpaceRadius &&
                    radius > kMinLSpaceRadius - kEpsLSpaceRadius, "Attempted to set invalid radius!");

                float rescaleFactor = lsp.Radius / radius;

                bool isSoi = IsSphereOfInfluence();
                bool isInfluencing = !ParentObj().Object().Influence.IsNull() &&
                    radius <= ParentObj().Object().Influence.LSpace().Radius;

                // Update local space attribute
                lsp.Radius = radius;
                lsp.MetersPerRadius = (double)radius * (Height() == 1
                    ? GetRootLSpaceNode().LSpace().MetersPerRadius
                    : m_Ctx->m_LSpaces[m_Ctx->m_Tree.GetGrandparent(m_NodeId)].MetersPerRadius);
                if (isSoi || isInfluencing) {
                    lsp.Primary = *this; /* an influencing space is its own Primary space */
                }
                else {
                    lsp.Primary = ParentObj().PrimaryLsp(); /* a non-influencing space's Primary is that of its parent object*/
                }
                //lsp.Grav = kGravitational * PrimaryObj().Object().State.Mass * pow(lsp.MetersPerRadius, -3.0); /* depends on Primary ! */
                lsp.Grav = LocalGravitationalParameter(PrimaryObj().Object().State.Mass, lsp.MetersPerRadius);

                // Move child objects to next-higher space if necessary
                std::vector<ObjectNode> childObjs = {};
                GetLocalObjects(childObjs);

                LSpaceNode prevLspNode = { node.PrevSibling };
                bool promoteAll = !prevLspNode.IsNull() && radius > prevLspNode.LSpace().Radius;

                for (auto objNode : childObjs) {
                    objNode.Object().State.Position *= rescaleFactor;
                    objNode.Object().State.Velocity *= rescaleFactor;

                    if (promoteAll ||
                        sqrtf(objNode.Object().State.Position.SqrMagnitude()) > kLocalSpaceEscapeRadius)
                    {
                        PromoteObjectNode(objNode); /* "promoting" still works because we haven't yet re-sorted the local space amongst its siblings */
                    }
                    else {
                        ComputeStateValidity(objNode);
                        TryComputeAttributes(objNode);
                        SubtreeCascadeAttributeChanges(objNode.Id());
                    }
                }

                // Resort the local space in its sibling linked-list
                if (rescaleFactor < 1.f) {
                    // Radius increased: sort node left-wards
                    while (!prevLspNode.IsNull()) {
                        if (radius > prevLspNode.LSpace().Radius) {
                            if (isSoi) {
                                prevLspNode.LSpace().Primary = prevLspNode; /* if resorting the sphere of influence, any smaller local spaces are now influencing */
                            }
                            m_Ctx->m_Tree.SwapWithPrevSibling(m_NodeId);
                            prevLspNode = { node.PrevSibling };
                        }
                        else break;
                    }
                }
                else {
                    // Radius decreased: sort node right-wards
                    LSpaceNode nextLspNode = { node.NextSibling };
                    while (!nextLspNode.IsNull()) {
                        if (radius < nextLspNode.LSpace().Radius) {
                            if (isSoi) {
                                nextLspNode.LSpace().Primary = ParentObj().PrimaryLsp(); /* if resorting the sphere of influence, any larger local spaces are no longer influencing */
                            }
                            m_Ctx->m_Tree.SwapWithNextSibling(m_NodeId);
                            nextLspNode = { node.NextSibling };
                        }
                        else break;
                    }
                }

                // Local space ordering potentially changed
                CallChildLSpacesChangedCallback(ParentObj()); /* local space itself has been fully changed at this point - call relevant callback before changing local objects */

                // Child objects potentially moved
                for (auto objNode : childObjs) {
                    if (objNode.ParentLsp() != *this) {
                        CallParentLSpaceChangedCallback(objNode);
                    }
                }

                // Adopt any child objects from the new next-higher local space
                LSpaceNode nextHigherSpace = NextHigherLSpace();
                childObjs.clear();
                nextHigherSpace.GetLocalObjects(childObjs);
                bool nextHigherIsSibling = nextHigherSpace.m_NodeId == node.PrevSibling;
                float radiusInPrev = lsp.Radius / nextHigherSpace.LSpace().Radius;
                Vector3 const& lspPos = ParentObj().Object().State.Position;
                for (auto objNode : childObjs)
                {
                    if (objNode.m_NodeId == m_Ctx->m_Tree[m_NodeId].Parent) continue; /* skip parent object */

                    if (nextHigherIsSibling && sqrtf(objNode.Object().State.Position.SqrMagnitude()) < radiusInPrev) {
                        DemoteObjectNode(objNode);

                        CallParentLSpaceChangedCallback(objNode);
                    }
                    else if (!nextHigherIsSibling && sqrtf((objNode.Object().State.Position - lspPos).SqrMagnitude()) < lsp.Radius) {
                        DemoteObjectNode(*this, objNode);

                        CallParentLSpaceChangedCallback(objNode);
                    }
                }

                // Finally, update orbits with all local space changes
                if (isSoi) {
                    SubtreeCascadeAttributeChanges(ParentObj().m_NodeId); /* if SOI changed, update all sibling spaces */
                }
                else {
                    SubtreeCascadeAttributeChanges(m_NodeId);
                }
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

        struct Integration
        {
            enum class Method {
                Angular = 0,
                Linear = 1
            };

            Method Method = Method::Angular; /* set to Angular by default in TryComputeAttributes() anyway */
        private:
            friend class OrbitalPhysics;

            double PrevDT = 0.0;
            double UpdateTimer = 0.0;
            float DeltaTrueAnomaly = 0.f;

            ObjectNode UpdateNext = {};
        };

        /*** Attributes ***/
    public:
        struct Object
        {
            Validity Validity = Validity::InvalidParent;
            State State;
            Integration Integration;

            LSpaceNode Influence = {}; /* Local space node representing this object's sphere of influence: Null if object is not influencing */
            TId Orbit; /* ID of current orbit section, stored in Context::Orbits */
        };

        struct LocalSpace
        {
        public:
            float Radius = 0.f; /* Measured in parent's influence */
            double MetersPerRadius = 0.f;
            double Grav = 0.f;  /* Gravitational parameter (mu) */

            LSpaceNode Primary = {};
        };

        struct Elements
        {
            //double Grav = 0.f;          /* Gravitational parameter (mu) */
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

            //float TrueAnomaly = 0.f;

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


        /*** Orbit data ***/
    public:
        struct OrbitSection
        {
            LSpaceNode LocalSpace;    // The local space through which this orbit section describes its object's motion
            Elements Elements;        // Orbital motion description (shape, duration, etc)
            float TrueAnomaly = 0.f;  // Object's current true anomaly in this orbit section (if this section is the object's current)
            float TaEntry    = 0.f;   // True anomaly of orbit's point of entry into the local space (if the section escapes the local space, otherwise has value 0)
            float TaExit   = PI2f;  // True anomaly of orbit's point of escape from the local space (if the section escapes the local space, otherwise has value 2Pi)
            TId Next = NNull;         // Reference to next orbit section which will describe the object's motion after escaping this, or entering a new, local space (if this section escapes its local space or intersects another, otherwise has value NNull)
        };

        // OrbitalPhysics is the manager of all orbits - it is responsible for simulating orbital motion
        // I.e, we don't need a separate manager class, just a storage object in Context manipulated by static methods (as with nodes)


        /*** Node helpers ***/
    private:
        static ObjectNode NewObjectNode(LSpaceNode parentNode)
        {
            TNodeId newNodeId = m_Ctx->m_Tree.New(parentNode.Id());
            auto& object = m_Ctx->m_Objects.Add(newNodeId);
            object.Orbit = NewOrbit(parentNode);
            return ObjectNode{ newNodeId };
        }

        static void RemoveObjectNode(ObjectNode objNode)
        {
            DeleteOrbit(objNode.Object().Orbit);
            m_Ctx->m_Objects.Remove(objNode.Id());
            m_Ctx->m_Dynamics.TryRemove(objNode.Id());
            m_Ctx->m_Tree.Remove(objNode.Id());
        }

        static void RescaleLocalSpaces(ObjectNode objNode, float rescalingFactor)
        {
            double const& parentLspMetersPerRadius = objNode.ParentLsp().LSpace().MetersPerRadius;
            std::vector<LSpaceNode> lspNodes = {};
            objNode.GetLocalSpaces(lspNodes);
            for (auto lspNode : lspNodes)
            {
                auto& lsp = lspNode.LSpace();
                lsp.Radius *= rescalingFactor;
                lsp.MetersPerRadius = parentLspMetersPerRadius * (double)lsp.Radius;
            }
        }

        /// <summary>
        /// Moves object from its current local space to the next-higher local space, recomputing relative position to preserve absolute position.
        /// </summary>
        static void PromoteObjectNode(ObjectNode objNode)
        {
            LSpaceNode oldLspNode = objNode.ParentLsp();
            LV_CORE_ASSERT(!oldLspNode.IsRoot(), "Cannot promote objects in the root local space!");
            LSpaceNode newLspNode = oldLspNode.NextHigherLSpace();

            float rescalingFactor;
            Object& obj = objNode.Object();
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

            m_Ctx->m_Tree.Move(objNode.Id(), newLspNode.Id());

            objNode.Orbit().LocalSpace = newLspNode; // TEMPORARY ! TODO - use orbit sections to facilitate promotion/demotion

            RescaleLocalSpaces(objNode, rescalingFactor);
            ComputeStateValidity(objNode);
            TryComputeAttributes(objNode);
            SubtreeCascadeAttributeChanges(objNode.Id());
        }

        /// <summary>
        /// Moves object to a lower local space which is attached to another object in the same current local space.
        /// </summary>
        static void DemoteObjectNode(LSpaceNode newLspNode, ObjectNode objNode)
        {
            LV_CORE_ASSERT(newLspNode.ParentLsp() == objNode.ParentLsp(), "The given local space is not in the same local space as the given object!");

            float rescalingFactor = 1.f / newLspNode.LSpace().Radius;

            auto const& parentObj = newLspNode.ParentObj().Object();
            auto& obj = objNode.Object();
            obj.State.Position = (obj.State.Position - parentObj.State.Position) * rescalingFactor;
            obj.State.Velocity = (obj.State.Velocity - parentObj.State.Velocity) * rescalingFactor;

            m_Ctx->m_Tree.Move(objNode.Id(), newLspNode.Id());

            objNode.Orbit().LocalSpace = newLspNode; // TEMPORARY ! TODO - use orbit sections to facilitate promotion/demotion

            RescaleLocalSpaces(objNode, rescalingFactor);
            ComputeStateValidity(objNode);
            TryComputeAttributes(objNode);
            SubtreeCascadeAttributeChanges(objNode.Id());
        }

        /// <summary>
        /// Moves object to the next-lower local space attached to the same object as the current local space.
        /// </summary>
        /// <param name="objNode"></param>
        static void DemoteObjectNode(ObjectNode objNode)
        {
            LSpaceNode lspNode = objNode.ParentLsp();
            LSpaceNode newLspNode = { lspNode.Node().NextSibling };
            LV_CORE_ASSERT(!newLspNode.IsNull(), "There is no next-lower local space!");

            float rescalingFactor = lspNode.LSpace().Radius / newLspNode.LSpace().Radius;

            auto& obj = objNode.Object();
            obj.State.Position *= rescalingFactor;
            obj.State.Velocity *= rescalingFactor;

            m_Ctx->m_Tree.Move(objNode.Id(), newLspNode.Id());

            objNode.Orbit().LocalSpace = newLspNode; // TEMPORARY ! TODO - use orbit sections to facilitate promotion/demotion

            RescaleLocalSpaces(objNode, rescalingFactor);
            ComputeStateValidity(objNode);
            TryComputeAttributes(objNode);
            SubtreeCascadeAttributeChanges(objNode.Id());
        }

        static LSpaceNode NewLSpaceNode(ObjectNode parentNode, float radius = kDefaultLSpaceRadius)
        {
            TNodeId newLspNodeId = { m_Ctx->m_Tree.New(parentNode.Id()) };
            m_Ctx->m_LSpaces.Add(newLspNodeId).Radius = 1.f;
            LSpaceNode newLspNode = { newLspNodeId };
            newLspNode.SetRadius(radius);
            return newLspNode;
        }

        static LSpaceNode NewSoiNode(ObjectNode parentNode, float radiusOfInfluence)
        {
            LV_CORE_ASSERT(parentNode.Object().Influence.IsNull(), "Object already has sphere of influence!");
            TNodeId newSoiNodeId = { m_Ctx->m_Tree.New(parentNode.Id()) };
            m_Ctx->m_LSpaces.Add(newSoiNodeId).Radius = 1.f;
            LSpaceNode newSoiNode = { newSoiNodeId };
            parentNode.Object().Influence = newSoiNode;
            newSoiNode.SetRadiusImpl(radiusOfInfluence);
            return newSoiNode;
        }

        static void RemoveLSpaceNode(LSpaceNode lspNode)
        {
            m_Ctx->m_LSpaces.Remove(lspNode.Id());
            m_Ctx->m_Tree.Remove(lspNode.Id());
        }


        /*** Orbit helpers ***/
    private:
        static TId NewOrbit(LSpaceNode lspNode)
        {
            TId newFirstSectionId = m_Ctx->m_OrbitSections.New();
            m_Ctx->m_OrbitSections.Get(newFirstSectionId).LocalSpace = lspNode;
            return newFirstSectionId;
        }

        // Deletes an orbit (a linked list of orbit sections) from the given section
        static void DeleteOrbit(TId sectionId)
        {
            while (sectionId != NNull)
            {
                TId nextSection = m_Ctx->m_OrbitSections.Get(sectionId).Next;
                m_Ctx->m_OrbitSections.Erase(sectionId);
                sectionId = nextSection;
            }
        }

        static void ComputeOrbit(TId firstSectionId, Vector3 const& localPosition, Vector3d const& localVelocity, size_t maxSections = 1)
        {
            TId sectionId = firstSectionId;
            for (size_t i = 0; i < maxSections; i++)
            {
                auto& section = m_Ctx->m_OrbitSections.Get(sectionId);
                ComputeElements(section, localPosition, localVelocity);
                ComputeTaLimits(section);
                if (section.TaExit == PI2f) { break; }

                break;
                // TODO : add new section(s) and loop over
            }
        }

        // Computes the true anomalies of the orbit's local entry and escape points
        static void ComputeTaLimits(OrbitSection& section)
        {
            auto& elems = section.Elements;

            if (section.LocalSpace.IsInfluencing())
            {
                float apoapsisRadius = elems.P / (1.f - elems.E);
                bool escapesLocalSpace = elems.Type == OrbitType::Hyperbola || apoapsisRadius > kLocalSpaceEscapeRadius;

                section.TaEntry = 0.f;
                section.TaExit = PI2f;
                if (escapesLocalSpace) {
                    section.TaExit = acosf((elems.P / kLocalSpaceEscapeRadius - 1.f) / elems.E);
                    section.TaEntry = PI2f - section.TaExit;
                }
            }
            else
            {
                // TODO : compute escapes for non-local orbits !!!
                section.TaEntry = 0.f;
                section.TaExit = PI2f;
            }
        }

        // Populates an orbit section's elements, and computes its current true anomaly from the given position
        static void ComputeElements(OrbitSection& section, Vector3 const& localPosition, Vector3d const& localVelocity)
        {
            auto& elems = section.Elements;
            auto& lsp = section.LocalSpace.LSpace();

            Vector3 positionFromPrimary = localPosition + section.LocalSpace.LocalOffsetFromPrimary();
            Vector3d velocityFromPrimary = localVelocity + section.LocalSpace.LocalVelocityFromPrimary();
            
            // TODO : get local velocity relative to primary !!! (velocity may, e.g, be zero relative to local parent but non-zero relative to a non-local primary)

            Vector3d Hvec = Vector3d(positionFromPrimary).Cross(velocityFromPrimary);
            double H2 = Hvec.SqrMagnitude();
            elems.H = sqrt(H2);
            if (elems.H == 0)
            {
                /* handle position or velocity being zero */
                elems = Elements();
                section.TrueAnomaly = 0.f;
                return;
            }
            elems.PerifocalNormal = (Vector3)(Hvec / elems.H);

            /* Loss of precision due to casting is acceptable: semi-latus rectum is on the order of 1 in all common cases, due to distance parameterisation */
            elems.P = (float)(H2 / lsp.Grav);
            elems.VConstant = lsp.Grav / elems.H;

            /* Loss of precision due to casting is acceptable: result of vector division (V x H / Grav) is on the order of 1 */
            Vector3 posDir = positionFromPrimary.Normalized();
            Vector3 Evec = (Vector3)(velocityFromPrimary.Cross(Hvec) / lsp.Grav) - posDir;
            float e2 = Evec.SqrMagnitude();
            elems.E = sqrtf(e2);

            float e2term;
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
                e2term += kEps; /* guarantees e2term > 0 */
            }

            // Dimensions
            elems.SemiMajor = elems.P / e2term;
            elems.SemiMinor = elems.SemiMajor * sqrtf(e2term);

            elems.C = elems.P / (1.f + elems.E);
            elems.C += (elems.Type == OrbitType::Hyperbola) ? elems.SemiMajor : -elems.SemiMajor; /* different center positions for elliptical and hyperbolic */

            elems.T = PI2 * (double)(elems.SemiMajor * elems.SemiMinor) / elems.H;

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

            // Current true anomaly
            section.TrueAnomaly = AngleBetweenUnitVectors(elems.PerifocalX, posDir);
            // Disambiguate based on whether the position is in the positive or negative Y-axis of the perifocal frame
            if (posDir.Dot(elems.PerifocalY) < 0.f) {
                // Velocity is in the negative X-axis of the perifocal frame
                section.TrueAnomaly = PI2f - section.TrueAnomaly;
            }
        }

        /*** Simulation resources ***/
    public:
        class Context
        {
            friend class OrbitalPhysics;

            Tree m_Tree;
            Storage<OrbitSection> m_OrbitSections;

            AttributeStorage<Object> m_Objects;
            AttributeStorage<LocalSpace> m_LSpaces;
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
                rootObj.Influence.m_NodeId = kRootLspId;

                auto& rootLsp = m_LSpaces.Add(kRootLspId);
                rootLsp.Radius = 1.f;
                rootLsp.Primary.m_NodeId = kRootLspId; /* an influencing lsp is its own primary */
            }
            Context(Context const& other) = default;
            ~Context()
            {
                // Estimating required memory allocation for converting vectors to static arrays
                LV_CORE_INFO("OrbitalPhysics final tree size: {0} ({1} objects, {2} local spaces)",
                    m_Tree.Size(), m_Objects.Size(), m_LSpaces.Size());
            }
        public:
            std::function<void(ObjectNode)> m_ParentLSpaceChangedCallback;
            std::function<void(ObjectNode)> m_ChildLSpacesChangedCallback;
        };

        static void SetContext(Context* ctx) { m_Ctx = ctx; }
    private:
        inline static Context* m_Ctx = nullptr;

        static constexpr TNodeId kRootObjId = 0;
        static constexpr TNodeId kRootLspId = 1;

        /*** Simulation helpers ***/
    private:
        static void CallParentLSpaceChangedCallback(ObjectNode objNode)
        {
            if (m_Ctx->m_ParentLSpaceChangedCallback) {
                m_Ctx->m_ParentLSpaceChangedCallback(objNode);
            }
            else {
                LV_WARN("Callback function 'ParentLSpaceChangedCallback' is not set in this context!");
            }
        }

        static void CallChildLSpacesChangedCallback(ObjectNode objNode)
        {
            if (m_Ctx->m_ChildLSpacesChangedCallback) {
                m_Ctx->m_ChildLSpacesChangedCallback(objNode);
            }
            else {
                LV_WARN("Callback function 'ChildLSpacesChangedCallback' is not set in this context!");
            }
        }

        static bool ValidPosition(ObjectNode objNode)
        {
            static constexpr float kEscapeDistance2 = kLocalSpaceEscapeRadius * kLocalSpaceEscapeRadius;

            if (objNode.IsRoot()) return true;

            float posMag2 = objNode.Object().State.Position.SqrMagnitude();
            float posFromPrimaryMag2 = objNode.LocalPositionFromPrimary().SqrMagnitude();
            return posMag2 < kEscapeDistance2 && posFromPrimaryMag2 > 0;
                /* TODO : check for influence overlaps */
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

            if (!objNode.ParentLsp().IsInfluencing()) { // <--------------------------------------------- TEMPORARY !!!
                // TODO : foreign (non-local) orbits
                LV_WARN("Orbits in non-influencing local spaces are not yet supported!");
                return false;
            }

            if (LSpaceNode(kRootLspId).LSpace().MetersPerRadius == 0.0) {
                LV_WARN("OrbitalPhysics root scaling has not been set!");
                return false;
            }
            if (LSpaceNode(kRootLspId).LSpace().Grav == 0.0) {
                LV_WARN("OrbitalPhysics root object mass has not been set!");
                return false;
            }
            return objNode.ParentObj().Object().Validity == Validity::Valid;
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
            auto& elems = objNode.Orbit().Elements;
            return elems.P / (1.f + elems.E * cosf(trueAnomaly));
        }

        static Vector3 ObjectPositionAtTrueAnomaly(ObjectNode objNode, float trueAnomaly)
        {
            auto& elems = objNode.Orbit().Elements;
            float radiusAtTrueAnomaly = OrbitEquation(objNode, trueAnomaly);
            Vector3 directionAtTrueAnomaly = cosf(trueAnomaly) * elems.PerifocalX
                + sinf(trueAnomaly) * elems.PerifocalY;
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
            float massFactor = (float)pow(obj.State.Mass / objNode.PrimaryObj().Object().State.Mass, 0.4);
            float radiusOfInfluence = objNode.Orbit().Elements.SemiMajor * massFactor;
            if (radiusOfInfluence > kMinLSpaceRadius)
            {
                if (radiusOfInfluence > kMaxLSpaceRadius) {
                    LV_WARN("Sphere of influence is too wide - adjust orbit radius or object mass!");
                    obj.Validity = Validity::InvalidPath;
                    return;
                }
                if (obj.Influence.IsNull()) {
                    NewSoiNode(objNode, radiusOfInfluence);
                }
                else {
                    obj.Influence.SetRadiusImpl(radiusOfInfluence);
                    LV_CORE_ASSERT(obj.Influence.LSpace().Primary == obj.Influence, "Sphere of influence should still be its own Primary!");
                }
                LV_CORE_ASSERT(!obj.Influence.IsNull() && m_Ctx->m_LSpaces.Has(obj.Influence.m_NodeId), "Failed to create sphere of influence!");
            }
            else if (!obj.Influence.IsNull()) {
                // Object was previously influencing - remove old sphere of influence
                CollapseLocalSpace(obj.Influence);
                obj.Influence = { NNull };
            }
        }

#ifdef EXCLUDE
        static void ComputeDynamics(ObjectNode objNode)
        {
            LV_CORE_ASSERT(!objNode.IsRoot(), "Cannot compute dynamics on root object!");

            auto& obj = objNode.Object();
            auto& elems = objNode.Orbit().Elements;

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

        static void ComputeOrbit(ObjectNode objNode)
        {
            LV_CORE_ASSERT(!objNode.IsRoot(), "Cannot compute elements on root object!");

            auto& obj = objNode.Object();

            auto& elems = objNode.Elements();

            auto lspNode = objNode.ParentLsp();
            auto& lsp = lspNode.LSpace();

            LV_CORE_ASSERT(obj.Validity == Validity::Valid || obj.Validity == Validity::InvalidPath,
                "Cannot compute elements on an object with invalid parent, mass, or position!");

            //elems.Grav = kGravitational * objNode.PrimaryObj().Object().State.Mass * pow(lsp.MetersPerRadius, -3.0);
            LV_CORE_ASSERT(false, "TODO: compute LocalSpace::Grav!");

            Vector3 positionFromPrimary = objNode.LocalPositionFromPrimary();
            // TODO : get local velocity relative to primary !!! (velocity may be zero relative to local parent, but non-zero relative to a non-local primary)

            Vector3d Hvec = Vector3d(positionFromPrimary).Cross(obj.State.Velocity);
            double H2 = Hvec.SqrMagnitude();
            elems.H = sqrt(H2);
            if (elems.H == 0)
            {
                /* handle position or velocity being zero */
                elems = Elements();
                return;
            }
            elems.PerifocalNormal = (Vector3)(Hvec / elems.H);

            /* Loss of precision due to casting is acceptable: semi-latus rectum is on the order of 1 in all common cases, due to distance parameterisation */
            elems.P = (float)(H2 / lsp.Grav);
            elems.VConstant = lsp.Grav / elems.H;

            /* Loss of precision due to casting is acceptable: result of vector division (V x H / Grav) is on the order of 1 */
            Vector3 posDir = positionFromPrimary.Normalized();
            Vector3 Evec = (Vector3)(obj.State.Velocity.Cross(Hvec) / lsp.Grav) - posDir;
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
#endif


        inline static double ComputeObjDT(double velocityMagnitude, double minDT = kDefaultMinDT)
        {
            if (velocityMagnitude > 0) {
                return std::max(kMaxUpdateDistanced / velocityMagnitude, minDT);
            }
            return minDT;
        }


        static void TryComputeAttributes(ObjectNode objNode)
        {
            LV_CORE_ASSERT(!objNode.IsRoot(), "Cannot compute orbit or influence on root object!");

            UpdateQueueSafeRemove(objNode);

            auto& obj = objNode.Object();
            if (obj.Validity == Validity::Valid || obj.Validity == Validity::InvalidPath)
            {
                LV_CORE_ASSERT(objNode.Orbit().LocalSpace == objNode.ParentLsp(), "Orbit/hierarchy local space mismatch!");

                obj.Validity = Validity::Valid;
                ComputeOrbit(obj.Orbit, obj.State.Position, obj.State.Velocity);
                ComputeInfluence(objNode);
                if (!objNode.IsDynamic() &&
                    objNode.Orbit().Elements.Type == OrbitType::Hyperbola)
                {
                    obj.Validity = Validity::InvalidPath;
                }

                /* TEMP */
                if (!objNode.ParentLsp().IsInfluencing()) { obj.Validity = Validity::InvalidPath; } // TODO : non-local orbits !!!
                /* /TEMP */

                if (obj.Validity == Validity::Valid)
                {
                    UpdateQueuePushFront(objNode);

                    obj.Integration.PrevDT = ComputeObjDT(sqrt(obj.State.Velocity.SqrMagnitude()));
                    Vector3 positionFromPrimary = objNode.LocalPositionFromPrimary();
                    float posMag2 = positionFromPrimary.SqrMagnitude();
                    obj.Integration.DeltaTrueAnomaly = (float)(obj.Integration.PrevDT * objNode.Orbit().Elements.H) / posMag2;

                    // TODO : handle cases where dynamic acceleration is non-zero, e.g, bool isDynamicallyAccelerating = m_Dynamics.Has(object) && !m_Dynamics[object].ContAcceleration.IsZero()
                    if (obj.Integration.DeltaTrueAnomaly > kMinUpdateTrueAnomaly) {
                        obj.Integration.Method = Integration::Method::Angular;
                    }
                    else {
                        Vector3 posDir = positionFromPrimary / sqrtf(posMag2);
                        obj.State.Acceleration = -(Vector3d)posDir * objNode.ParentLsp().LSpace().Grav / (double)posMag2;
                        if (objNode.IsDynamic()) {
                            obj.State.Acceleration += objNode.Dynamics().ContAcceleration;
                        }
                        obj.Integration.Method = Integration::Method::Linear;
                    }
                }
            }
        }


        /// <summary>
        /// Returns gravitational parameter (GM/r in standard units) scaled to a local space with the given length unit.
        /// </summary>
        static double LocalGravitationalParameter(double localPrimaryMass, double localMetersPerUnitLength)
        {
            return kGravitational * localPrimaryMass * pow(localMetersPerUnitLength, -3.0);
        }


        /// <summary>
        /// Returns speed for a circular orbit around the local primary (not circular in local space if local space is not influencing) at the given distance from the primary (measured in local space radii).
        /// Assumes orbiter has insignificant mass compared to primary.
        /// </summary>
        static double CircularOrbitSpeed(LSpaceNode lspNode, float localRadius)
        {
            /* ||V_circular|| = sqrt(mu / ||r||), where mu is the gravitational parameter of the orbit */
            return sqrt(lspNode.LSpace().Grav / (double)localRadius);
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
            if (rMag == 0) { return Vector3d::Zero(); }

            Vector3 rDir =  positionFromPrimary / rMag;
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
                if (IsLocalSpace(nodeId)) {
                    LSpaceNode subLspNode{ nodeId };
                    if (!subLspNode.IsRoot() && !subLspNode.IsSphereOfInfluence()) {
                        subLspNode.SetRadius(subLspNode.LSpace().Radius); /* recomputes MetersPerRadius and Grav */
                    }
                }
                else {
                    // TODO : preserve orbit shapes ?
                    ObjectNode subObjNode{ nodeId };
                    ComputeStateValidity(subObjNode);
                    TryComputeAttributes(subObjNode);
                }
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
                auto& lsp = m_Ctx->m_UpdateQueueFront.ParentLsp().LSpace();
                auto& obj = m_Ctx->m_UpdateQueueFront.Object();
                auto& orbit = m_Ctx->m_UpdateQueueFront.Orbit();
                auto& elems = orbit.Elements;
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
                        obj.State.Acceleration = -(Vector3d)posDir * lsp.Grav / (double)posMag2;
                        if (isDynamic) {
                            obj.State.Acceleration += m_Ctx->m_UpdateQueueFront.Dynamics().ContAcceleration;
                        }
                        obj.Integration.Method = Integration::Method::Linear;
                        LV_CORE_TRACE("Object {0} switched from angular to linear integration!", m_Ctx->m_UpdateQueueFront.m_NodeId);
                        // NOTE: switch falls through to case Integration::Method::Linear
                    }
                    else {
                        orbit.TrueAnomaly += obj.Integration.DeltaTrueAnomaly;
                        orbit.TrueAnomaly = Wrapf(orbit.TrueAnomaly, PI2f);

                        // Compute new state
                        float sinT = sinf(orbit.TrueAnomaly);
                        float cosT = cosf(orbit.TrueAnomaly);
                        float r = elems.P / (1.f + elems.E * cosT); /* orbit equation: r = h^2 / mu * 1 / (1 + e * cos(trueAnomaly)) */

                        //Vector3 positionFromPrimary = r * (cosT * elems.PerifocalX + sinT * elems.PerifocalY);
                        //Vector3d velocityFromPrimary = elems.VConstant * (Vector3d)((elems.E + cosT) * elems.PerifocalY - sinT * elems.PerifocalX);
                        //obj.State.Position = positionFromPrimary - m_Ctx->m_UpdateQueueFront.ParentLsp().LocalOffsetFromPrimary();
                        //obj.State.Velocity = velocityFromPrimary - m_Ctx->m_UpdateQueueFront.ParentLsp().LocalVelocityFromPrimary();

                        /* state according to elements (local distance scaling, relative to primary) */
                        obj.State.Position = r * (cosT * elems.PerifocalX + sinT * elems.PerifocalY);
                        obj.State.Velocity = elems.VConstant * (Vector3d)((elems.E + cosT) * elems.PerifocalY - sinT * elems.PerifocalX);
                        /* state relative to local space */
                        LSpaceNode parentLspNode = m_Ctx->m_UpdateQueueFront.ParentLsp();
                        obj.State.Position -= parentLspNode.LocalOffsetFromPrimary();
                        obj.State.Velocity -= parentLspNode.LocalVelocityFromPrimary();

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
                    Vector3d newAcceleration = -(Vector3d)positionFromPrimary * lsp.Grav / (double)(r2 * sqrtf(r2));
                    bool isDynamicallyAccelerating = false;
                    if (isDynamic) {
                        newAcceleration += m_Ctx->m_UpdateQueueFront.Dynamics().ContAcceleration;
                        isDynamicallyAccelerating = !m_Ctx->m_UpdateQueueFront.Dynamics().ContAcceleration.IsZero();
                    }
                    obj.State.Velocity += 0.5 * (obj.State.Acceleration + newAcceleration) * objDT;
                    obj.State.Acceleration = newAcceleration;

                    if (isDynamicallyAccelerating) {
                        ComputeOrbit(obj.Orbit, obj.State.Position, obj.State.Velocity);
                        ComputeInfluence(m_Ctx->m_UpdateQueueFront);
                    }
                    else {
                        // Update true anomaly with new position vector
                        // Code taken from ComputeElements():
                        Vector3 posDir = positionFromPrimary.Normalized();
                        float newTrueAnomaly = AngleBetweenUnitVectors(elems.PerifocalX, posDir);
                        if (posDir.Dot(elems.PerifocalY) < 0.f && newTrueAnomaly != 0.f) {
                            newTrueAnomaly = PI2f - newTrueAnomaly;
                        }

                        // Not dynamically accelerating, so we ensure true anomaly does not decrease:
                        float dTrueAnomaly = newTrueAnomaly - orbit.TrueAnomaly;
                        if (dTrueAnomaly < -PIf) {
                            orbit.TrueAnomaly = newTrueAnomaly; /* True anomaly has wrapped around at periapsis in the forwards direction */
                        }
                        else if (!(dTrueAnomaly > PIf)) {
                            orbit.TrueAnomaly = std::max(newTrueAnomaly, orbit.TrueAnomaly); /* True anomaly has NOT wrapped at periapsis in the backwards direction (we can safely take the larger value) */
                        }
                        /* else, true anomaly has wrapped backwards at periapsis so we discard the new value */
                    }

                    // Recheck integration method choice
                    objDT = ComputeObjDT(sqrt(obj.State.Velocity.SqrMagnitude()), minObjDT);
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
                LV_CORE_ASSERT(orbit.TrueAnomaly < PI2f, "True anomaly has not been accurately wrapped to range [0, 2Pi)!");

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

                    if (orbit.TrueAnomaly > orbit.TaExit) {
                        LV_CORE_ASSERT(sqrtf(obj.State.Position.SqrMagnitude()) > kLocalSpaceEscapeRadius - kEps, "False positive on escape test!");
                        LV_CORE_ASSERT(!m_Ctx->m_UpdateQueueFront.ParentLsp().IsRoot(), "Cannot escape root local space!");

                        PromoteObjectNode(m_Ctx->m_UpdateQueueFront);
                        CallParentLSpaceChangedCallback(m_Ctx->m_UpdateQueueFront);

                        LV_CORE_ASSERT(obj.Validity == Validity::Valid, "Invalid dynamics after escape!");

                        // Prepare integration in new local space
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
                            obj.State.Acceleration = -(Vector3d)posDir * lsp.Grav / (double)posMag2;
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
            auto& rootLsp = LSpaceNode(kRootLspId).LSpace();
            rootLsp.MetersPerRadius = meters;
            rootLsp.Grav = LocalGravitationalParameter(ObjectNode(kRootObjId).Object().State.Mass, meters);

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
        static ObjectNode Create(LSpaceNode lspNode, double mass, Vector3 const& position, bool dynamic = false)
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

        /// <summary>
        /// Deletes a local space, moving any objects within to the next higher local space such that their absolute positions/velocities are preserved.
        /// </summary>
        /// <param name="lspNode"></param>
        static void CollapseLocalSpace(LSpaceNode lspNode)
        {
            std::vector<ObjectNode> localObjs = {};
            lspNode.GetLocalObjects(localObjs);
            for (auto objNode : localObjs) {
                PromoteObjectNode(objNode);
            }
            LV_CORE_ASSERT(m_Ctx->m_Tree[lspNode.m_NodeId].FirstChild == OrbitalPhysics::NNull, "Failed to remove all children!");

            ObjectNode parentObjNode = lspNode.ParentObj();

            RemoveLSpaceNode(lspNode);

            CallChildLSpacesChangedCallback(parentObjNode);

            for (auto objNode : localObjs) {
                CallParentLSpaceChangedCallback(objNode);
            }
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
