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
        static constexpr float kMaxPositionStep = 1e-6f; /* largest delta position we can allow before integration step becomes too visible (for when object DT is much greater than frame DT) */
        static constexpr double kMaxPositionStepd = (double)kMaxPositionStep;
        static constexpr double kMaxVelocityStep = kMaxPositionStepd / 10.0;
        static constexpr double kMinUpdateTrueAnomaly = ::std::numeric_limits<double>::epsilon() * 1e3f; /* smallest delta true anomaly we can allow before precision error becomes unacceptable for long-term angular integration */
        ////////////////////////////////////////


        /*** General-purpose array-based storage class ***/
    private:
        using TId = uint32_t;
        static constexpr TId IdNull = ::std::numeric_limits<TId>::max();

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
        struct State;
        struct Motion;
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
                    LV_CORE_ASSERT(m_Ctx->m_States.Has(nodeId), "Object node must have a State attribute!");
                    LV_CORE_ASSERT(nodeId == kRootObjId || m_Ctx->m_Motions.Has(nodeId), "Object node must have a Motion attribute!");
                }
            }

            TNodeId Id() const { return m_NodeId; }

            /* For OrbitalPhysics/internal use*/
        private:
            Node const& Node() const { return m_Ctx->m_Tree[m_NodeId]; }
            int Height() const { return m_Ctx->m_Tree.Height(m_NodeId); }
            Object& Object() const { return m_Ctx->m_Objects[m_NodeId]; }
            State& State() const { return m_Ctx->m_States[m_NodeId]; }
            Motion& Motion() const { return m_Ctx->m_Motions[m_NodeId]; }
            Dynamics& Dynamics() const { return m_Ctx->m_Dynamics[m_NodeId]; }

            OrbitSection& Orbit() const
            {
                TId orbitId = m_Ctx->m_Motions[m_NodeId].Orbit;
                LV_CORE_ASSERT(orbitId != IdNull, "Object does not have an Orbit!");
                return m_Ctx->m_OrbitSections[orbitId];
            }

            /* For user application/external use */
        public:
            static constexpr ObjectNode NNull() { return {}; }
            bool IsNull() const { return m_NodeId == OrbitalPhysics::NNull; }
            bool IsRoot() const { return m_NodeId == kRootObjId; }
            bool IsDynamic() const { return m_Ctx->m_Dynamics.Has(m_NodeId); }
            bool IsInfluencing() const { return !Object().Influence.IsNull(); }
            bool HasChildLSpace() const { return m_Ctx->m_Tree[m_NodeId].FirstChild != IdNull; }

            OrbitalPhysics::Object const& GetObj() const { return Object(); }
            OrbitalPhysics::State const& GetState() const { return State(); }
            OrbitalPhysics::Motion const& GetMotion() const { return Motion(); }
            OrbitalPhysics::Dynamics const& GetDynamics() const { return Dynamics(); }

            // Computes or updates the Orbit and returns its first section.
            OrbitalPhysics::OrbitSection const& GetOrbit(size_t maxSections = 1) const
            {
                auto& state = m_Ctx->m_States[m_NodeId];
                auto& motion = m_Ctx->m_Motions[m_NodeId];
                if (motion.Orbit == IdNull) {
                    motion.Orbit = NewOrbit(ParentLsp());
                    ComputeOrbit(motion.Orbit, state.Position, state.Velocity, maxSections);
                }
                else if (motion.Integration == Motion::Integration::Linear) {
                    motion.TrueAnomaly = Orbit().Elements.TrueAnomalyOf(state.Position.Normalized());
                }
                return m_Ctx->m_OrbitSections[motion.Orbit];
            }

            LSpaceNode ParentLsp() const { return LSpaceNode{ m_Ctx->m_Tree.GetParent(m_NodeId) }; }
            ObjectNode ParentObj() const { return ObjectNode{ m_Ctx->m_Tree.GetGrandparent(m_NodeId) }; }

            LSpaceNode PrimaryLsp() const { return m_Ctx->m_LSpaces[m_Ctx->m_Tree.GetParent(m_NodeId)].Primary; }
            ObjectNode PrimaryObj() const { return m_Ctx->m_LSpaces[m_Ctx->m_Tree.GetParent(m_NodeId)].Primary.ParentObj(); }

            LSpaceNode FirstChildLSpace() const { return LSpaceNode{ m_Ctx->m_Tree[m_NodeId].FirstChild }; }
            LSpaceNode SphereOfInfluence() const { return m_Ctx->m_Objects[m_NodeId].Influence; }

            Vector3 LocalPositionFromPrimary() const
            {
                return m_Ctx->m_States[m_NodeId].Position +
                    LSpaceNode(m_Ctx->m_Tree[m_NodeId].Parent).LocalOffsetFromPrimary();
            }

            Vector3d LocalVelocityFromPrimary() const
            {
                return m_Ctx->m_States[m_NodeId].Velocity +
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

                m_Ctx->m_Tree.Move(m_NodeId, newLspNode.m_NodeId);

                TryPrepareObject(*this);
                TryPrepareSubtree(*this);
            }

            void SetMass(double mass) const
            {
                LV_ASSERT(!IsNull(), "Cannot set mass of null object!");

                m_Ctx->m_States[m_NodeId].Mass = mass;
                if (IsRoot()) {
                    auto& rootLsp = Object().Influence.LSpace();
                    rootLsp.Grav = LocalGravitationalParameter(mass, rootLsp.MetersPerRadius);
                    // TODO : exclude from release builds ?
                }
                TryPrepareObject(*this);
                TryPrepareSubtree(*this);
            }

            void SetPosition(Vector3 const& position) const
            {
                LV_ASSERT(!IsNull() && !IsRoot(), "Cannot set position of root or null object!");

                m_Ctx->m_States[m_NodeId].Position = position;
                TryPrepareObject(*this);
                TryPrepareSubtree(*this);
            }

            void SetVelocity(Vector3d const& velocity) const
            {
                LV_ASSERT(!IsNull() && !IsRoot(), "Cannot set velocity of root or null object!");

                m_Ctx->m_States[m_NodeId].Velocity = velocity;
                TryPrepareObject(*this);
                TryPrepareSubtree(*this);
            }

            /// <summary>
            /// Returns velocity for a circular counter-clockwise orbit around the object's current primary, given its current mass and position.
            /// </summary>
            /// <param name="object">Physics object ID</param>
            Vector3d CircularOrbitVelocity() const
            {
                return OrbitalPhysics::CircularOrbitVelocity(
                    LSpaceNode{ m_Ctx->m_Tree[m_NodeId].Parent }, m_Ctx->m_States[m_NodeId].Position);
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
                TryPrepareObject(*this);
            }

            void SetContinuousAcceleration(Vector3d const& acceleration) const
            {
                LV_ASSERT(!IsDynamic(), "Cannot set dynamic acceleration on non-dynamic objects!");

                auto& state = State();
                auto& motion = Motion();
                auto& dynamics = Dynamics();

                dynamics.ContAcceleration = acceleration;
                if (acceleration.IsZero()) return;

                motion.Integration = Motion::Integration::Dynamic;
                motion.PrevDT -= motion.UpdateTimer;
                motion.UpdateTimer = 0.0;
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
            bool IsHighestLSpaceOnObject() const { return m_Ctx->m_Tree[m_NodeId].PrevSibling == OrbitalPhysics::NNull; }
            bool IsLowestLSpaceOnObject() const { return m_Ctx->m_Tree[m_NodeId].NextSibling == OrbitalPhysics::NNull; }
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

            LSpaceNode UpperLSpace() const
            {
                TNodeId prevSibling = m_Ctx->m_Tree[m_NodeId].PrevSibling;
                return LSpaceNode{ prevSibling == OrbitalPhysics::NNull
                    ? m_Ctx->m_Tree.GetGrandparent(m_NodeId) : prevSibling };
            }

            LSpaceNode InnerLSpace() const
            {
                return { m_Ctx->m_Tree[m_NodeId].NextSibling }; /* returns Null LSpace if no inner local space exists! */
            }

            float InnerLSpaceLocalRadius() const
            {
                return IsLowestLSpaceOnObject() ? 0.f : InnerLSpace().LSpace().Radius / LSpace().Radius;
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
                return (m_Ctx->m_States[lspParentObjId].Position +
                    LocalOffsetFromPrimary(m_Ctx->m_Tree[lspParentObjId].Parent, primaryLspId))
                    / m_Ctx->m_LSpaces[lspId].Radius;
            }

            Vector3d LocalVelocityFromPrimary(TNodeId lspId, TNodeId primaryLspId) const
            {
                LV_CORE_ASSERT(m_Ctx->m_Tree.Height(lspId) % 2 == 1 && m_Ctx->m_Tree.Height(primaryLspId) % 2 == 1, "Invalid IDs!");
                if (lspId == primaryLspId) return Vector3d::Zero();

                TNodeId lspParentObjId = m_Ctx->m_Tree[lspId].Parent;
                return (m_Ctx->m_States[lspParentObjId].Velocity +
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
                lsp.Grav = LocalGravitationalParameter(PrimaryObj().State().Mass, lsp.MetersPerRadius);

                // Move child objects to next-higher space if necessary
                std::vector<ObjectNode> childObjs = {};
                GetLocalObjects(childObjs);

                LSpaceNode prevLspNode = { node.PrevSibling };
                bool promoteAll = !prevLspNode.IsNull() && radius > prevLspNode.LSpace().Radius;

                for (auto objNode : childObjs) {
                    objNode.State().Position *= rescaleFactor;
                    objNode.State().Velocity *= rescaleFactor;

                    if (promoteAll ||
                        sqrtf(objNode.State().Position.SqrMagnitude()) > kLocalSpaceEscapeRadius)
                    {
                        PromoteObjectNode(objNode); /* "promoting" still works because we haven't yet re-sorted the local space amongst its siblings */
                    }
                    else {
                        TryPrepareObject(objNode);
                        TryPrepareSubtree(objNode.m_NodeId);
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
                LSpaceNode nextHigherSpace = UpperLSpace();
                childObjs.clear();
                nextHigherSpace.GetLocalObjects(childObjs);
                bool nextHigherIsSibling = nextHigherSpace.m_NodeId == node.PrevSibling;
                float radiusInPrev = lsp.Radius / nextHigherSpace.LSpace().Radius;
                Vector3 const& lspPos = ParentObj().State().Position;
                for (auto objNode : childObjs)
                {
                    if (objNode.m_NodeId == m_Ctx->m_Tree[m_NodeId].Parent) continue; /* skip parent object */

                    if (nextHigherIsSibling && sqrtf(objNode.State().Position.SqrMagnitude()) < radiusInPrev) {
                        DemoteObjectNode(objNode);

                        CallParentLSpaceChangedCallback(objNode);
                    }
                    else if (!nextHigherIsSibling && sqrtf((objNode.State().Position - lspPos).SqrMagnitude()) < lsp.Radius) {
                        DemoteObjectNode(*this, objNode);

                        CallParentLSpaceChangedCallback(objNode);
                    }
                }

                // Finally, update orbits with all local space changes
                if (isSoi) {
                    TryPrepareSubtree(ParentObj().m_NodeId); /* if SOI changed, update all sibling spaces */
                }
                else {
                    TryPrepareSubtree(m_NodeId);
                }
            }
        };

        /*** Object enums ***/
    public:
        enum class Validity
        {
            InvalidParent = 0,
            InvalidSpace,
            InvalidMass,
            InvalidPosition,
            InvalidMotion,
            Valid = 100
        };

        static std::string ValidityToString(Validity v)
        {
            switch (v)
            {
            case Validity::InvalidParent:   return "InvalidParent";
            case Validity::InvalidSpace:    return "InvalidSpace";
            case Validity::InvalidMass:     return "InvalidMass";
            case Validity::InvalidPosition: return "InvalidPosition";
            case Validity::InvalidMotion:   return "InvalidMotion";
            case Validity::Valid:           return "Valid";
            }
        }

        enum class OrbitType
        {
            Circle = 0,
            Ellipse = 1,
            Hyperbola = 2
        };

        /*** Attributes ***/
    public:
        struct Object
        {
            Validity Validity = Validity::InvalidParent;
            LSpaceNode Influence = {}; /* Local space node representing this object's sphere of influence: Null if object is not influencing */
        };

        struct State
        {
            double Mass = 0.0;
            Vector3 Position = { 0.f };
            Vector3d Velocity = { 0.0 };
            Vector3d Acceleration = { 0.0 };
        };

        struct Motion
        {
            enum class Integration {
                Angular = 0,
                Linear,
                Dynamic
            };
            Integration Integration = Integration::Angular;
            bool ForceLinear = false;
            double TrueAnomaly = 0.f;
        private:
            friend class OrbitalPhysics;

            double PrevDT = 0.0;
            double UpdateTimer = 0.0;
            double DeltaTrueAnomaly = 0.f;
            ObjectNode UpdateNext = {};

            TId Orbit = IdNull;
        };

        struct Dynamics
        {
            Vector3d ContAcceleration = { 0.0 }; /* Acceleration assumed to be constant between timesteps */
            Vector3d DeltaPosition = { 0.0 };
        };

        struct LocalSpace
        {
            float Radius = 0.f; /* Measured in parent's influence */
            double MetersPerRadius = 0.f;
            double Grav = 0.f;  /* Gravitational parameter (mu) */

            LSpaceNode Primary = {};
        };

        /*** Orbit data ***/
    public:
        class Elements
        {
        public:
            double H = 0.0;             /* Orbital specific angular momentum */
            float E = { 0.f };          /* Eccentricity */
            double VConstant = 0.f;     /* Constant factor of orbital velocity:     mu / h      */

            OrbitType Type = OrbitType::Circle; /* Type of orbit - defined by eccentricity, indicates the type of shape which describes the orbit path */

            /* Dimensions */
            float SemiMajor = 0.f, SemiMinor = 0.f; /* Semi-major and semi-minor axes */
            float C = 0.f;              /* Signed distance from occupied focus to centre, measured along perifocal frame's x-axis */
            double T = 0.0;             /* Orbit period, measured in seconds */
            float P = 0.f;              /* Orbit parameter, or semi-latus rectum:   h^2 / mu    */

            /* Orientation */
            float I = 0.f;              /* Inclination */
            Vector3 N = { 0.f };        /* Direction of ascending node */
            float Omega = 0.f;          /* Right ascension of ascending node */
            float ArgPeriapsis = 0.f;   /* Argument of periapsis */

            /* Perifocal frame */
            Vector3 PerifocalX = { 0.f }, PerifocalY = { 0.f }, PerifocalNormal = { 0.f };
            Quaternion PerifocalOrientation; /* Orientation of the perifocal frame relative to the reference frame */

        public:
            float RadiusAt(float trueAnomaly) const
            {
                return P / (1.f + E * cosf(trueAnomaly));
            }

            Vector3 PositionAt(float trueAnomaly) const
            {
                Vector3 directionAtTrueAnomaly = cosf(trueAnomaly) * PerifocalX
                    + sinf(trueAnomaly) * PerifocalY;
                return RadiusAt(trueAnomaly) * directionAtTrueAnomaly;
            }

            float TrueAnomalyOf(Vector3 const& positionDirection) const
            {
                LV_ASSERT(abs(positionDirection.SqrMagnitude() - 1.f) < 10.f * kEps, "Direction vector must be a unit vector (length was {0}, must be 1)!",
                    abs(positionDirection.SqrMagnitude() - 1.f));

                float trueAnomaly = AngleBetweenUnitVectorsf(PerifocalX, positionDirection);
                // Disambiguate based on whether the position is in the positive or negative Y-axis of the perifocal frame
                if (positionDirection.Dot(PerifocalY) < 0.f) {
                    // Velocity is in the negative X-axis of the perifocal frame
                    trueAnomaly = PI2f - trueAnomaly;
                }
                return trueAnomaly;
            }
        };

        class OrbitSection
        {
        public:
            LSpaceNode LocalSpace;      // The local space through which this orbit section describes its object's motion
            Elements Elements;          // Orbital motion description (shape, duration, etc)
            float TaEntry   = 0.f;      // True anomaly of orbit's point of entry into the local space (if the section escapes the local space, otherwise has value 0)
            float TaExit    = PI2f;     // True anomaly of orbit's point of escape from the local space (if the section escapes the local space, otherwise has value 2Pi)
            TId Next = NNull;           // Reference to next orbit section which will describe the object's motion after escaping this, or entering a new, local space (if this section escapes its local space or intersects another, otherwise has value NNull)

        public:
            Vector3 LocalPositionAt(float trueAnomaly) const
            {
                return Elements.PositionAt(trueAnomaly) - LocalSpace.LocalOffsetFromPrimary();
            }
        };

        // OrbitalPhysics is the manager of all orbits - it is responsible for simulating orbital motion
        // I.e, we don't need a separate manager class, just a storage object in Context manipulated by static methods (as with nodes)


        /*** Node helpers ***/
    private:
        static ObjectNode NewObjectNode(LSpaceNode parentNode)
        {
            TNodeId newNodeId = m_Ctx->m_Tree.New(parentNode.m_NodeId);
            m_Ctx->m_Objects.Add(newNodeId);
            m_Ctx->m_States.Add(newNodeId);
            m_Ctx->m_Motions.Add(newNodeId);
            return ObjectNode{ newNodeId };
        }

        static void RemoveObjectNode(ObjectNode objNode)
        {
            m_Ctx->m_Dynamics.TryRemove(objNode.m_NodeId);
            if (objNode.Motion().Orbit != IdNull) { DeleteOrbit(objNode.Motion().Orbit); }
            m_Ctx->m_Motions.Remove(objNode.m_NodeId);
            m_Ctx->m_States.Remove(objNode.m_NodeId);
            m_Ctx->m_Objects.Remove(objNode.m_NodeId);
            m_Ctx->m_Tree.Remove(objNode.m_NodeId);
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
            LSpaceNode newLspNode = oldLspNode.UpperLSpace();

            float rescalingFactor;
            State& state = objNode.State();
            if (oldLspNode.IsHighestLSpaceOnObject()) {
                rescalingFactor = oldLspNode.LSpace().Radius;
                state.Position = (state.Position * rescalingFactor) + oldLspNode.ParentObj().State().Position;
                state.Velocity = (state.Velocity * (double)rescalingFactor) + oldLspNode.ParentObj().State().Velocity;
            }
            else {
                rescalingFactor = oldLspNode.LSpace().Radius / newLspNode.LSpace().Radius;
                state.Position = state.Position * rescalingFactor;
                state.Velocity = state.Velocity * (double)rescalingFactor;
            }

            m_Ctx->m_Tree.Move(objNode.m_NodeId, newLspNode.m_NodeId);

            objNode.Orbit().LocalSpace = newLspNode; // TEMPORARY ! TODO - use orbit sections to facilitate promotion/demotion

            RescaleLocalSpaces(objNode, rescalingFactor);
            TryPrepareObject(objNode);
            TryPrepareSubtree(objNode.m_NodeId);
        }

        /// <summary>
        /// Moves object to a lower local space which is attached to another object in the same current local space.
        /// </summary>
        static void DemoteObjectNode(LSpaceNode newLspNode, ObjectNode objNode)
        {
            LV_CORE_ASSERT(newLspNode.ParentLsp() == objNode.ParentLsp(), "The given local space is not in the same local space as the given object!");

            float rescalingFactor = 1.f / newLspNode.LSpace().Radius;

            auto const& parentState = newLspNode.ParentObj().State();
            auto& state = objNode.State();
            state.Position = (state.Position - parentState.Position) * rescalingFactor;
            state.Velocity = (state.Velocity - parentState.Velocity) * rescalingFactor;

            m_Ctx->m_Tree.Move(objNode.m_NodeId, newLspNode.m_NodeId);

            objNode.Orbit().LocalSpace = newLspNode; // TEMPORARY ! TODO - use orbit sections to facilitate promotion/demotion

            RescaleLocalSpaces(objNode, rescalingFactor);
            TryPrepareObject(objNode);
            TryPrepareSubtree(objNode.m_NodeId);
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

            auto& state = objNode.State();
            state.Position *= rescalingFactor;
            state.Velocity *= rescalingFactor;

            m_Ctx->m_Tree.Move(objNode.m_NodeId, newLspNode.m_NodeId);

            objNode.Orbit().LocalSpace = newLspNode; // TEMPORARY ! TODO - use orbit sections to facilitate promotion/demotion

            RescaleLocalSpaces(objNode, rescalingFactor);
            TryPrepareObject(objNode);
            TryPrepareSubtree(objNode.m_NodeId);
        }

        static LSpaceNode NewLSpaceNode(ObjectNode parentNode, float radius = kDefaultLSpaceRadius)
        {
            TNodeId newLspNodeId = { m_Ctx->m_Tree.New(parentNode.m_NodeId) };
            m_Ctx->m_LSpaces.Add(newLspNodeId).Radius = 1.f;
            LSpaceNode newLspNode = { newLspNodeId };
            newLspNode.SetRadius(radius);
            return newLspNode;
        }

        static LSpaceNode NewSoiNode(ObjectNode parentNode, float radiusOfInfluence)
        {
            LV_CORE_ASSERT(parentNode.Object().Influence.IsNull(), "Object already has sphere of influence!");
            TNodeId newSoiNodeId = { m_Ctx->m_Tree.New(parentNode.m_NodeId) };
            m_Ctx->m_LSpaces.Add(newSoiNodeId).Radius = 1.f;
            LSpaceNode newSoiNode = { newSoiNodeId };
            parentNode.Object().Influence = newSoiNode;
            newSoiNode.SetRadiusImpl(radiusOfInfluence);
            return newSoiNode;
        }

        static void RemoveLSpaceNode(LSpaceNode lspNode)
        {
            m_Ctx->m_LSpaces.Remove(lspNode.m_NodeId);
            m_Ctx->m_Tree.Remove(lspNode.m_NodeId);
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
        static void DeleteOrbit(TId& sectionId)
        {
            while (sectionId != IdNull)
            {
                TId nextSection = m_Ctx->m_OrbitSections.Get(sectionId).Next;
                m_Ctx->m_OrbitSections.Erase(sectionId);
                sectionId = nextSection;
            }
            sectionId = IdNull;
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

            section.TaEntry = 0.f;
            section.TaExit = PI2f;
            if (false /*section.LocalSpace.IsInfluencing()*/)
            {
                // Local space escape
                float apoapsisRadius = elems.SemiMajor * (1.f + elems.E);
                if (elems.Type == OrbitType::Hyperbola || apoapsisRadius > kLocalSpaceEscapeRadius) {
                    section.TaExit = acosf((elems.P / kLocalSpaceEscapeRadius - 1.f) / elems.E);
                    section.TaEntry = PI2f - section.TaExit;
                }
                // Inner space entry
                else if (!section.LocalSpace.IsLowestLSpaceOnObject())
                {
                    float periapsisRadius = elems.SemiMajor * (1.f - elems.E);
                    float innerSpaceRelativeRadius = section.LocalSpace.InnerLSpace().LSpace().Radius / section.LocalSpace.LSpace().Radius;
                    if (periapsisRadius < innerSpaceRelativeRadius) {
                        section.TaEntry = acosf((elems.P / innerSpaceRelativeRadius - 1.f) / elems.E);
                        section.TaExit = PI2f - section.TaEntry;
                    }
                }
            }
            else
            {
                // TODO : get ta limits in primary space
                auto primarySpace = section.LocalSpace.PrimaryLsp();
                float primarySpaceRelativeScaling = primarySpace.LSpace().MetersPerRadius / section.LocalSpace.LSpace().MetersPerRadius;
                float escapeRadius = kLocalSpaceEscapeRadius * primarySpaceRelativeScaling;

                // COPY FROM ABOVE (+ edits)
                // Check for local space escape
                float apoapsisRadius = elems.SemiMajor * (1.f + elems.E);
                if (elems.Type == OrbitType::Hyperbola || apoapsisRadius > escapeRadius)
                {
                    section.TaExit = acosf((elems.P / escapeRadius - 1.f) / elems.E);
                    section.TaEntry = PI2f - section.TaExit;
                }
                // Check for inner space entry
                else if (!primarySpace.IsLowestLSpaceOnObject())
                {
                    float periapsisRadius = elems.SemiMajor * (1.f - elems.E);
                    float innerSpaceRelativeRadius = primarySpace.InnerLSpace().LSpace().Radius / primarySpace.LSpace().Radius * primarySpaceRelativeScaling;
                    if (periapsisRadius < innerSpaceRelativeRadius) {
                        section.TaEntry = acosf((elems.P / innerSpaceRelativeRadius - 1.f) / elems.E);
                        section.TaExit = PI2f - section.TaEntry;
                    }
                }
            }
        }

        // Populates an orbit section's elements, and computes its current true anomaly from the given position
        static void ComputeElements(OrbitSection& section, Vector3 const& localPosition, Vector3d const& localVelocity)
        {
            auto& elems = section.Elements;
            auto& lsp = section.LocalSpace.LSpace();

            Vector3 positionFromPrimary = localPosition + section.LocalSpace.LocalOffsetFromPrimary();
            Vector3d velocityFromPrimary = localVelocity + section.LocalSpace.LocalVelocityFromPrimary();

            Vector3d Hvec = Vector3d(positionFromPrimary).Cross(velocityFromPrimary);
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
            elems.ArgPeriapsis = AngleBetweenUnitVectorsf(elems.N, elems.PerifocalX);
            if (elems.N.Dot(elems.PerifocalY) > 0.f) {
                elems.ArgPeriapsis = PI2f - elems.ArgPeriapsis;
            }
            elems.PerifocalOrientation =
                Quaternion(elems.PerifocalNormal, elems.ArgPeriapsis)
                * Quaternion(elems.N, elems.I)
                * Quaternion(kReferenceNormal, elems.Omega);
        }

        /*** Simulation resources ***/
    public:
        class Context
        {
            friend class OrbitalPhysics;

            Tree m_Tree;
            Storage<OrbitSection> m_OrbitSections;

            AttributeStorage<Object> m_Objects;
            AttributeStorage<State> m_States;
            AttributeStorage<Motion> m_Motions;
            AttributeStorage<Dynamics> m_Dynamics;
            AttributeStorage<LocalSpace> m_LSpaces;

            ObjectNode m_UpdateQueueFront = {};
        public:
            Context()
            {
                m_Tree.New(); /* kRootObjId (0) */
                m_Tree.New(); /* kRootLspId (1) */
                LV_CORE_ASSERT(m_Tree.Has(kRootObjId), "Context failed to create root object node!");
                LV_CORE_ASSERT(m_Tree.Has(kRootLspId), "Context failed to create root local space node!");

                auto& rootObj = m_Objects.Add(kRootObjId);
                rootObj.Validity = Validity::InvalidParent; /* use InvalidParent to signify that root SCALING has not been set, instead of making a unique enum value */
                rootObj.Influence.m_NodeId = kRootLspId;
                m_States.Add(kRootObjId);

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

        static void ComputeInfluence(ObjectNode objNode)
        {
            LV_CORE_ASSERT(!objNode.IsRoot(), "Cannot compute influence of root object!");

            Object& obj = objNode.Object();

            /* Radius of influence = a(m / M)^0.4
             * Semi-major axis must be in the order of 1,
             * so the order of ROI is determined by (m / M)^0.4 */
            float massFactor = (float)pow(objNode.State().Mass / objNode.PrimaryObj().State().Mass, 0.4);
            float radiusOfInfluence = objNode.GetOrbit().Elements.SemiMajor * massFactor;
            radiusOfInfluence = std::min(radiusOfInfluence, kMaxLSpaceRadius + kEps * kMaxLSpaceRadius); /* restrict size while still causing InvalidMotion */

            if (!objNode.IsDynamic() && radiusOfInfluence > kMinLSpaceRadius)
            {
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


        inline static double ComputeObjDT(double velocityMagnitude, double minDT = kDefaultMinDT)
        {
            if (velocityMagnitude > 0.0) {
                // If velocity (V) is greater than max update distance (MUD),
                // computed DT is less than 1; if DT is too small (resulting in too many updates per frame),
                // we take minDT instead and sacrifice accuracy to maintain framerate.
                // Otherwise, if V < MUD, then DT > 1 and is safe to use.
                return std::max(kMaxPositionStepd / velocityMagnitude, minDT);
            }
            return minDT;
        }

        inline static double ComputeDynamicObjDT(double velocityMagnitude, double accelerationMagnitude, double minDT = kDefaultMinDT)
        {
            if (accelerationMagnitude > 0.0) {
                return std::min(ComputeObjDT(velocityMagnitude),
                    std::max(kMaxVelocityStep / accelerationMagnitude, minDT));
            }
            return ComputeObjDT(velocityMagnitude);
        }


        /// <summary>
        /// Returns gravitational parameter (GM/r in standard units) scaled to a local space with the given length unit.
        /// </summary>
        static double LocalGravitationalParameter(double localPrimaryMass, double localMetersPerUnitLength)
        {
            return kGravitational * localPrimaryMass * pow(localMetersPerUnitLength, -3.0);
        }


        static void UpdateQueuePushFront(ObjectNode objNode)
        {
            if (m_Ctx->m_UpdateQueueFront.IsNull()) {
                m_Ctx->m_UpdateQueueFront = objNode;
                objNode.Motion().UpdateNext = ObjectNode::NNull();
            }
            else {
                objNode.Motion().UpdateNext = m_Ctx->m_UpdateQueueFront;
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
                m_Ctx->m_UpdateQueueFront = objNode.Motion().UpdateNext;
                objNode.Motion().UpdateNext = ObjectNode::NNull();
                return;
            }
            ObjectNode queueItem = m_Ctx->m_UpdateQueueFront,
                queueNext = m_Ctx->m_UpdateQueueFront.Motion().UpdateNext;
            while (queueNext != objNode) {
                LV_CORE_ASSERT(!queueNext.IsNull(), "UpdateQueueRemove() could not find the given object in the update queue!");
                queueItem = queueNext;
                queueNext = queueNext.Motion().UpdateNext;
            }
            queueItem.Motion().UpdateNext = objNode.Motion().UpdateNext;
            objNode.Motion().UpdateNext = ObjectNode::NNull();
        }

        /// <summary>
        /// Removes the given object from the update queue, if it exists in the update queue.
        /// </summary>
        /// <returns>True if object was found and removed, false otherwise.</returns>
        static bool UpdateQueueSafeRemove(ObjectNode objNode)
        {
            if (m_Ctx->m_UpdateQueueFront.IsNull()) return false;
            if (m_Ctx->m_UpdateQueueFront == objNode) {
                m_Ctx->m_UpdateQueueFront = objNode.Motion().UpdateNext;
                objNode.Motion().UpdateNext = ObjectNode::NNull();
                return true;
            }
            ObjectNode queueItem = m_Ctx->m_UpdateQueueFront,
                queueNext = m_Ctx->m_UpdateQueueFront.Motion().UpdateNext;
            while (!queueNext.IsNull()) {
                if (queueNext == objNode) {
                    queueItem.Motion().UpdateNext = objNode.Motion().UpdateNext;
                    objNode.Motion().UpdateNext = ObjectNode::NNull();
                    return true;
                }
                queueItem = queueNext;
                queueNext = queueNext.Motion().UpdateNext;
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
            auto& motion = objNode.Motion();

            ObjectNode queueItem = motion.UpdateNext;
            if (queueItem.IsNull()) return;
            if (motion.UpdateTimer < queueItem.Motion().UpdateTimer) return;
            m_Ctx->m_UpdateQueueFront = queueItem;

            ObjectNode queueNext = queueItem.Motion().UpdateNext;
            while (!queueNext.IsNull()) {
                if (motion.UpdateTimer < queueNext.Motion().UpdateTimer) break;
                queueItem = queueNext;
                queueNext = queueNext.Motion().UpdateNext;
            }
            queueItem.Motion().UpdateNext = objNode;
            motion.UpdateNext = queueNext;
        }


        static Validity TryPrepareObject(ObjectNode objNode)
        {
            UpdateQueueSafeRemove(objNode);

            auto& obj = objNode.Object();

            obj.Validity = Validity::Valid;
            if (!ValidParent(objNode)) {
                return obj.Validity = Validity::InvalidParent;
            }
            else if (!ValidSpace(objNode)) {
                return obj.Validity = Validity::InvalidSpace;
            }
            else if (!ValidMass(objNode)) {
                return obj.Validity = Validity::InvalidMass;
            }
            else if (!ValidPosition(objNode)) {
                return obj.Validity = Validity::InvalidPosition;
            }

            if (objNode.IsRoot()) {
                // Root object does not have motion or dynamically computed influence - we return Valid here
                return Validity::Valid;
            }

            // State is valid: prepare Motion
            ComputeMotion(objNode);

            // Prepare Influence
            ComputeInfluence(objNode);

            if (!ValidMotion(objNode)) {
                return obj.Validity = Validity::InvalidMotion;
            }

            if (obj.Validity == Validity::Valid)
            {
                UpdateQueuePushFront(objNode);
            }

            return obj.Validity;
        }

        static bool ValidMotion(ObjectNode objNode)
        {
            if (!objNode.IsDynamic()) {
                auto& orbit = objNode.GetOrbit();
                if (orbit.TaExit < PI2) {
                    LV_WARN("Object {0} has invalid motion: non-dynamic objects cannot exit their local space!");
                    return false;
                }
                if (objNode.IsInfluencing()) {
                    float roi = objNode.SphereOfInfluence().LSpace().Radius;
                    if (roi > kMaxLSpaceRadius) {
                        LV_WARN("Object {0} has invalid motion: sphere of influence is too wide - adjust orbit radius or object mass!",
                            objNode.m_NodeId);
                        return false;
                    }
                    if (orbit.Elements.RadiusAt(PIf) + roi > kLocalSpaceEscapeRadius ||
                        orbit.Elements.RadiusAt(0.f) - roi < objNode.ParentLsp().InnerLSpaceLocalRadius()) {
                        LV_WARN("Object {0} has invalid motion: sphere of influence is crossing local space boundaries!", objNode.m_NodeId);
                        return false;
                    }
                }
            }
            return true;
        }

        static bool ValidPosition(ObjectNode objNode)
        {
            static constexpr float kEscapeDistance2 = kLocalSpaceEscapeRadius * kLocalSpaceEscapeRadius - kEps;

            if (objNode.IsRoot()) return true;

            auto parentLsp = objNode.ParentLsp();

            float innerSpaceRadius = parentLsp.IsLowestLSpaceOnObject() ? 0.f : parentLsp.InnerLSpace().LSpace().Radius / parentLsp.LSpace().Radius;
            float posMag2 = objNode.State().Position.SqrMagnitude();
            float posFromPrimaryMag2 = objNode.LocalPositionFromPrimary().SqrMagnitude();

            if (posFromPrimaryMag2 < kEps) {
                LV_WARN("Object {0} has invalid position: distance from primary object {1} must be non-zero!", objNode.m_NodeId, objNode.PrimaryObj().m_NodeId);
                return false;
            }
            if (posMag2 > kEscapeDistance2 || posMag2 < innerSpaceRadius * innerSpaceRadius + kEps) {
                LV_WARN("Object {0} has invalid position: must be inside its local space!", objNode.m_NodeId, objNode.PrimaryObj().m_NodeId);
                return false;
            }

            std::vector<ObjectNode> siblings = {};
            parentLsp.GetLocalObjects(siblings);
            for (auto sibNode : siblings) {
                if (sibNode == objNode) continue;
                float separation = sqrtf((objNode.State().Position - sibNode.State().Position).SqrMagnitude());
                float minSeparation = kEps + (sibNode.HasChildLSpace() ? sibNode.FirstChildLSpace().LSpace().Radius : 0.f);
                if (separation < minSeparation) {
                    LV_WARN("Object {0} has invalid position: overlapping with another object's {1} local space!", objNode.m_NodeId, sibNode.m_NodeId);
                    return false;
                }
            }

            return true;
        }

        static bool ValidMass(ObjectNode objNode)
        {
            static constexpr double kMaxCOG = 1e-4; /* Maximum offset for shared centre of gravity with a separation distance of 1 */

            auto& state = objNode.State();

            if (state.Mass <= 0.0) {
                LV_WARN("Object {0} has invalid mass: mass must be positive (non-zero)!", objNode.m_NodeId);
                return false;
            }
            if (objNode.IsRoot()) { return true; }
            double& primaryMass = objNode.PrimaryObj().State().Mass;
            double massRatio = state.Mass / (state.Mass + primaryMass);
            if (massRatio > kMaxCOG) {
                LV_WARN("Object {0} has invalid mass: ratio with primary object {1} mass is too high "
                    "(ratio is m / (m + M) = {2}, must be less than {3})!",
                    objNode.m_NodeId, objNode.PrimaryObj().m_NodeId, massRatio, kMaxCOG);
                return false;
            }
            static const float kMaxDynamicMassRatio = powf(kMinLSpaceRadius, 2.5f);
            massRatio = state.Mass / primaryMass;
            if (objNode.IsDynamic() && (float)massRatio > kMaxDynamicMassRatio) {
                LV_WARN("Object {0} has invalid mass: ratio with primary object {1} mass is too high for a dynamic object "
                    "(ratio is m/M = {2}, must be less than {3} for dynamic objects)!",
                    objNode.m_NodeId, objNode.PrimaryObj().m_NodeId, massRatio, kMaxDynamicMassRatio);
                return false;
            }
            return true;
        }

        static bool ValidSpace(ObjectNode objNode)
        {
            if (objNode.IsRoot()) { return true; }
            if (!objNode.IsDynamic() && !objNode.ParentLsp().IsInfluencing()) {
                LV_WARN("Object {0} invalid local space {1}: non-dynamic object cannot belong to a non-influencing space!", objNode.m_NodeId, objNode.ParentLsp().m_NodeId);
                return false;
            }
            return true;
        }

        static bool ValidParent(ObjectNode objNode)
        {
            if (objNode.IsRoot()) {
                return objNode.Object().Validity != Validity::InvalidParent; /* see SetRootSpaceScaling() */
            }
            if (objNode.ParentObj().Object().Validity != Validity::Valid) {
                LV_WARN("Object {0} invalid parent {1}: parent Validity must be Validity::Valid!", objNode.m_NodeId, objNode.ParentObj().m_NodeId);
                return false;
            }
            return true;
        }


        /// <summary>
        /// Runs TryPrepareObject() on every ObjectNode in the subtree rooted at the node with the given ID (excluding the root node itself).
        /// </summary>
        static void TryPrepareSubtree(TNodeId rootNodeId)
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
                    TryPrepareObject(subObjNode);
                }
            }
        }

        static double ApproximateDeltaTrueAnomaly(Vector3d const& posFromPrimary, double distFromPrimary, Vector3d const& velFromPrimary, double objDT)
        {
            double vHorz = sqrt(velFromPrimary.SqrMagnitude() - pow(velFromPrimary.Dot(posFromPrimary) / distFromPrimary, 2.0));
            return objDT * vHorz / distFromPrimary;
        }

        static enum class Motion::Integration SelectIntegrationMethod(double deltaTrueAnomaly, bool isDynamicallyAccelerating = false)
        {
            return !isDynamicallyAccelerating && deltaTrueAnomaly > kMinUpdateTrueAnomaly
                ? Motion::Integration::Angular : Motion::Integration::Linear;
        }

        static void ComputeMotion(ObjectNode objNode)
        {
            LV_CORE_ASSERT(!objNode.IsRoot(), "Root object cannot have Motion!");

            auto& state = objNode.State();
            auto& motion = objNode.Motion();

            if (motion.Orbit != IdNull) {
                DeleteOrbit(motion.Orbit);
            }

            motion.PrevDT = ComputeObjDT(sqrt(state.Velocity.SqrMagnitude()));

            double approxDTrueAnomaly;
            Vector3d posDir;
            double posMag2;
            bool isDynamicallyAccelerating;
            {
                // Approximate delta true anomaly (dTa) as the angle change in position direction caused by velocity across delta time (dT)
                Vector3d posFromPrimary = (Vector3d)objNode.LocalPositionFromPrimary();
                posMag2 = posFromPrimary.SqrMagnitude();
                double r = sqrt(posMag2);
                posDir = posFromPrimary / r;
                Vector3d velFromPrimary = objNode.LocalVelocityFromPrimary();
                double approxDTrueAnomaly = ApproximateDeltaTrueAnomaly(posFromPrimary, r, velFromPrimary, motion.PrevDT);

                isDynamicallyAccelerating = objNode.IsDynamic() && !objNode.Dynamics().ContAcceleration.IsZero();
                motion.Integration = SelectIntegrationMethod(approxDTrueAnomaly, isDynamicallyAccelerating);
            }
            switch (motion.Integration)
            {
            case Motion::Integration::Angular:
            {
                // Angular integration
                motion.Integration = Motion::Integration::Angular;
                auto& orbit = objNode.GetOrbit(); /* creates Orbit */
                motion.TrueAnomaly = orbit.Elements.TrueAnomalyOf((Vector3)posDir);
                motion.DeltaTrueAnomaly = (motion.PrevDT * orbit.Elements.H) / posMag2;
                break;
            }
            case Motion::Integration::Linear:
            {
                // Linear integration
                motion.Integration = Motion::Integration::Linear;
                state.Acceleration = -posDir * objNode.ParentLsp().LSpace().Grav / posMag2;
                if (isDynamicallyAccelerating) {
                    state.Acceleration += objNode.Dynamics().ContAcceleration;
                }
                break;
            }
            }
        }

        /*** Simulation usage ***/
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

            // Subtract elapsed time from all object timers
            ObjectNode objNode = m_Ctx->m_UpdateQueueFront;
            do {
                objNode.Motion().UpdateTimer -= dT;
                objNode = objNode.Motion().UpdateNext;
            } while (!objNode.IsNull());


            double minObjDT = dT / kMaxObjectUpdates;

            // Update all objects with timers less than 0
            ObjectNode& updateNode = m_Ctx->m_UpdateQueueFront;
            while (updateNode.Motion().UpdateTimer < 0.0)
            {
                auto lspNode = updateNode.ParentLsp();
                auto& lsp = lspNode.LSpace();
                auto& obj = updateNode.Object();
                auto& state = updateNode.State();
                auto& motion = updateNode.Motion();
                bool isDynamic = updateNode.IsDynamic();

#ifdef LV_DEBUG // debug object pre-update
                /*m_Stats.ObjStats[m_UpdateQueueFront].NumObjectUpdates += 1;
                float prevTrueAnomaly = elems.TrueAnomaly;*/
#endif

                double& objDT = motion.PrevDT;

                // Motion integration
                switch (motion.Integration)
                {
                case Motion::Integration::Angular:
                {
                    /* Integrate true anomaly:
                    * dTrueAnomaly / dT = h / r^2
                    * */
                    auto& orbit = updateNode.Orbit();
                    auto& elems = orbit.Elements;
                    motion.TrueAnomaly += motion.DeltaTrueAnomaly;
                    motion.TrueAnomaly = Wrapf(motion.TrueAnomaly, PI2f);

                    // Compute new state
                    float sinT = sinf((float)motion.TrueAnomaly);
                    float cosT = cosf((float)motion.TrueAnomaly);
                    float r = elems.P / (1.f + elems.E * cosT); /* orbit equation: r = h^2 / mu * 1 / (1 + e * cos(trueAnomaly)) */

                    /* state according to elements (local distance scaling, relative to primary) */
                    state.Position = r * (cosT * elems.PerifocalX + sinT * elems.PerifocalY);
                    state.Velocity = elems.VConstant * (Vector3d)((elems.E + cosT) * elems.PerifocalY - sinT * elems.PerifocalX);
                    /* state relative to local space */
                    LSpaceNode parentLspNode = updateNode.ParentLsp();
                    state.Position -= parentLspNode.LocalOffsetFromPrimary();
                    state.Velocity -= parentLspNode.LocalVelocityFromPrimary();

                    objDT = ComputeObjDT(sqrt(state.Velocity.SqrMagnitude()), minObjDT);
                    motion.DeltaTrueAnomaly = (objDT * elems.H) / (double)(r * r);

                    // Re-select integration method
                    motion.Integration = SelectIntegrationMethod(motion.DeltaTrueAnomaly);
                    if (motion.Integration == Motion::Integration::Linear)
                    {
                        // Prepare Linear integration
                        Vector3d positionFromPrimary = (Vector3d)updateNode.LocalPositionFromPrimary();
                        double posMag2 = positionFromPrimary.SqrMagnitude();
                        Vector3d posDir = positionFromPrimary / sqrt(posMag2);
                        state.Acceleration = -posDir * lsp.Grav / posMag2;
                        if (isDynamic) {
                            state.Acceleration += updateNode.Dynamics().ContAcceleration;
                        }
                        LV_CORE_TRACE("Object {0} switched to Linear integration!", updateNode.m_NodeId);
                    }

                    break;
                }
                case Motion::Integration::Linear:
                {
                    /* Velocity verlet :
                    * p1 = p0 + v0 * dT + 0.5 * a0 * dT^2
                    * a1 = (-rDirection) * G * M / r^2 + dynamicAcceleration
                    * v1 = v0 + 0.5 * (a0 + a1) * dT
                    * */
                    state.Position += (Vector3)(state.Velocity * objDT) + 0.5f * (Vector3)(state.Acceleration * objDT * objDT);
                    Vector3d positionFromPrimary = (Vector3d)updateNode.LocalPositionFromPrimary();
                    double r2 = positionFromPrimary.SqrMagnitude();
                    double r = sqrt(r2);

                    Vector3d newAcceleration = -positionFromPrimary * lsp.Grav / (r2 * r);
                    bool isDynamicallyAccelerating = false;
                    if (isDynamic) {
                        newAcceleration += updateNode.Dynamics().ContAcceleration;
                        isDynamicallyAccelerating = !updateNode.Dynamics().ContAcceleration.IsZero();
                    }
                    state.Velocity += 0.5 * (state.Acceleration + newAcceleration) * objDT;
                    state.Acceleration = newAcceleration;

                    objDT = ComputeObjDT(sqrt(state.Velocity.SqrMagnitude()), minObjDT);

                    if (isDynamicallyAccelerating && motion.Orbit != IdNull) {
                        // Dynamic acceleration invalidates orbit:
                        // Orbit was requested by the user in the previous frame so we need to delete the stored (now invalid) data
                        DeleteOrbit(motion.Orbit); /* We do not compute the orbit of a linearly integrated object until it is requested */
                    }

                    // Re-select integration method
                    double approxDTrueAnomaly = ApproximateDeltaTrueAnomaly(positionFromPrimary, r, updateNode.LocalVelocityFromPrimary(), objDT);
                    motion.Integration = SelectIntegrationMethod(approxDTrueAnomaly, isDynamicallyAccelerating);
                    if (motion.Integration == Motion::Integration::Angular)
                    {
                        // Prepare Angular integration
                        motion.DeltaTrueAnomaly = (motion.PrevDT * updateNode.GetOrbit().Elements.H) / r2; /* GetOrbit() creates or updates orbit */

                        LV_CORE_TRACE("Object {0} switched to Angular integration!", updateNode.m_NodeId);
                    }

                    break;
                }
                case Motion::Integration::Dynamic:
                {
                    auto& dynamics = updateNode.Dynamics();

                    if (motion.Orbit != IdNull) {
                        // Dynamic acceleration invalidates orbit:
                        // Orbit was requested by the user in the previous frame so we need to delete the stored (now invalid) data
                        DeleteOrbit(motion.Orbit); /* We do not compute the orbit of a linearly integrated object until it is requested */
                    }

                    dynamics.DeltaPosition += (state.Velocity * objDT) + (0.5 * state.Acceleration * objDT * objDT);

                    Vector3d positionFromPrimary = (Vector3d)updateNode.LocalPositionFromPrimary() + dynamics.DeltaPosition;
                    double r2 = positionFromPrimary.SqrMagnitude();
                    double r = sqrt(r2);
                    Vector3d newAcceleration = dynamics.ContAcceleration - (positionFromPrimary * lsp.Grav / (r2 * r));

                    state.Velocity += 0.5 * (state.Acceleration + newAcceleration) * objDT;
                    state.Acceleration = newAcceleration;

                    static constexpr double kMaxUpdateDistanced2 = kMaxPositionStepd * kMaxPositionStepd;
                    bool positionUpdated = false;
                    double deltaPosMag2 = dynamics.DeltaPosition.SqrMagnitude();
                    if (deltaPosMag2 > kMaxUpdateDistanced2) {
                        Vector3 dPosf = (Vector3)dynamics.DeltaPosition;
                        state.Position += dPosf;
                        dynamics.DeltaPosition -= (Vector3d)dPosf;

                        positionUpdated = true;
                    }

                    if (dynamics.ContAcceleration.IsZero())
                    {
                        double v = sqrt(state.Velocity.SqrMagnitude());
                        objDT = ComputeObjDT(v, minObjDT);
                        if (positionUpdated)
                        {
                            // Switch to Angular or Linear integration
                            double approxDTrueAnomaly = ApproximateDeltaTrueAnomaly(positionFromPrimary, r, updateNode.LocalVelocityFromPrimary(), objDT);
                            motion.Integration = SelectIntegrationMethod(approxDTrueAnomaly, false);
                            if (motion.Integration == Motion::Integration::Angular)
                            {
                                motion.DeltaTrueAnomaly = (motion.PrevDT * updateNode.GetOrbit().Elements.H) / r2; /* GetOrbit() creates or updates orbit */
                            }
                        }
                        else {
                            // Prepare the next integration step so that it jumps to the next position update.
                            objDT = std::max(minObjDT, objDT - (kMaxPositionStepd - sqrt(deltaPosMag2)) / v);
                        }
                    }
                    else
                    {
                        objDT = ComputeDynamicObjDT(sqrt(state.Velocity.SqrMagnitude()), sqrt(state.Acceleration.SqrMagnitude()), minObjDT);
                    }
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
                if (isDynamic)
                {
                    auto& dynamics = updateNode.Dynamics();

                    bool lspChanged = false;

                    // Local escape
                    float r = sqrtf(state.Position.SqrMagnitude());
                    if (r > kLocalSpaceEscapeRadius)
                    {
                        LV_CORE_ASSERT(!updateNode.ParentLsp().IsRoot(), "Cannot escape root local space!");
                        lspChanged = true;
                        PromoteObjectNode(updateNode);
                    }
                    // Inner space entry
                    else if (!lspNode.IsLowestLSpaceOnObject() &&
                        r < lspNode.InnerLSpace().LSpace().Radius / lsp.Radius)
                    {
                        lspChanged = true;
                        DemoteObjectNode(updateNode);
                    }
                    // Local subspace entry
                    else
                    {
                        std::vector<ObjectNode> objs{ };
                        lspNode.GetLocalObjects(objs);
                        for (auto objNode : objs)
                        {
                            if (objNode == updateNode) continue; /* skip self */
                            if (!objNode.HasChildLSpace()) continue; /* skip objs without subspaces */

                            auto subspaceNode = objNode.FirstChildLSpace();
                            float s = sqrtf((state.Position - objNode.State().Position).SqrMagnitude());
                            if (s < subspaceNode.LSpace().Radius)
                            {
                                lspChanged = true;
                                DemoteObjectNode(subspaceNode, updateNode);
                            }
                        }
                    }

                    if (lspChanged) {
                        LV_CORE_ASSERT(obj.Validity == Validity::Valid, "Invalid dynamics after event!");
                        CallParentLSpaceChangedCallback(updateNode);
                    }
                }

                motion.UpdateTimer += objDT;
                UpdateQueueSortFront();
            }

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
        /// Scaling is measured in meters per unit-radii of the root local space.
        /// E.g, a vector with length 1 in the root orbital space has a simulated length equal to the root scaling.
        /// </summary>
        /// <param name="meters">MUST be a non-zero positive number.</param>
        static void SetRootSpaceScaling(double meters)
        {
            meters = std::max(meters, 0.0);

            auto& rootLsp = LSpaceNode(kRootLspId).LSpace();
            rootLsp.MetersPerRadius = meters;
            rootLsp.Grav = LocalGravitationalParameter(ObjectNode(kRootObjId).State().Mass, meters);

            auto rootObjNode = ObjectNode(kRootObjId);
            auto& rootObj = rootObjNode.Object();
            rootObj.Validity = Validity::InvalidParent;
            if (meters > 0.0) {
                rootObj.Validity = ValidMass(rootObjNode) ? Validity::Valid : Validity::InvalidMass;
            }

            TryPrepareSubtree(kRootLspId);
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
            auto& state = newObjNode.State();
            state.Mass = mass;
            state.Position = position;
            state.Velocity = velocity;

            if (dynamic) {
                m_Ctx->m_Dynamics.Add(newObjNode.m_NodeId);
            }

            Validity validity = TryPrepareObject(newObjNode);
            LV_INFO("New OrbitalPhysics object ({0}) validity '{1}'", newObjNode.m_NodeId, ValidityToString(validity));

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
            return Create(lspNode, mass, position, CircularOrbitVelocity(lspNode, position), dynamic);
        }

        /// <summary>
        /// Create an uninitialised orbital physics object in the specified orbital space.
        /// </summary>
        /// <param name="userId">ID of the user-object to be associated with the created physics object</param>
        /// <returns>ID of the created physics object</returns>
        static ObjectNode Create(LSpaceNode lspNode, bool dynamic = false)
        {
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
            State& state = objNode.State();
            std::vector<LSpaceNode> lspaces;
            for (size_t i = 0; i < objNode.GetLocalSpaces(lspaces); i++)
            {
                float rescalingFactor = lspaces[i].LSpace().Radius;
                std::vector<ObjectNode> localObjs;
                for (size_t j = 0; j < lspaces[i].GetLocalObjects(localObjs); j++)
                {
                    State& childState = localObjs[j].State();
                    childState.Position = (childState.Position * rescalingFactor) + state.Position;
                    childState.Velocity = (childState.Velocity * (double)rescalingFactor) + state.Velocity;

                    m_Ctx->m_Tree.Move(localObjs[j].m_NodeId, parentLsp.m_NodeId);

                    TryPrepareObject(localObjs[j]);
                    TryPrepareSubtree(localObjs[j].m_NodeId);
                }
            }

            UpdateQueueSafeRemove(objNode);
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


        /* Query functions */
    public:
        /// <summary>
        /// Returns speed for a circular orbit around the local primary (not circular in local space if local space is non-influencing) at the given distance from the primary (measured in local space radii).
        /// Assumes orbiter has insignificant mass compared to primary.
        /// </summary>
        static double CircularOrbitSpeed(LSpaceNode lspNode, float localRadius)
        {
            /* ||V_circular|| = sqrt(mu / ||r||), where mu is the gravitational parameter of the orbit */
            return sqrt(lspNode.LSpace().Grav / (double)localRadius);
        }


        /// <summary>
        /// Returns velocity for a circular counter-clockwise orbit in the given local space, given an initial position.
        /// Assumes orbiter has insignificant mass compared to primary.
        /// </summary>
        static Vector3d CircularOrbitVelocity(LSpaceNode lspNode, Vector3 const& localPosition)
        {
            /* Keep the orbital plane as flat (close to the reference plane) as possible:
             * derive velocity direction as the cross product of reference normal and normalized position */
            Vector3d vDir;
            Vector3 positionFromPrimary = localPosition + lspNode.LocalOffsetFromPrimary();
            float rMag = sqrtf(positionFromPrimary.SqrMagnitude());
            if (rMag == 0) { return Vector3d::Zero(); }

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

    };

}
