#ifndef LV_NTREE_H
#define LV_NTREE_H

#include "Storage.h"

namespace Limnova
{

/// <summary>
/// Dynamic array-based tree structure for representing arbitrarily sized, ordered trees.
/// Every node has one parent, an ordered list of siblings, and a first child, with the exception of the root node which has no parent.
/// The tree can be empty. Only one node can exist at height 0 - the root node.
/// Calling New for the first time creates the root node; all subsequent calls to New must create nodes with heights greater than 0.
/// </summary>
class NTree
{
public:
    using TNodeId = uint32_t;

    static constexpr TNodeId NNull = ::std::numeric_limits<TNodeId>::max();

    /// <summary> Node class from which the NTree is constructed - its members are Node IDs describing its relationships with adjacent Nodes. </summary>
    struct Node
    {
        TNodeId Parent = NNull;
        TNodeId NextSibling = NNull;
        TNodeId PrevSibling = NNull;
        TNodeId FirstChild = NNull;
    };

    NTree();

    NTree(const NTree& rhs);

    /// <summary> Get the number of nodes in the tree. </summary>
    size_t Size() const;

    /// <summary> Check if the given ID corresponds to a node in the tree. </summary>
    /// <returns>True if there is a node in the tree with the given ID, otherwise false.</returns>
    bool Has(TNodeId nodeId) const;

    /// <summary>
    /// Adds a new node to the tree. If the tree is empty, the new node is the root node, otherwise the new node is parented to the root node.
    /// Re-uses a previously allocated Node object or, if none exists, constructs new.
    /// </summary>
    /// <returns>ID of the new node.</returns>
    TNodeId New();

    /// <summary>
    /// Adds a new node to the tree, parented to the node with the given ID. Cannot be called on an empty tree.
    /// Re-uses a previously allocated Node object or, if none exists, constructs new.
    /// </summary>
    /// <returns>ID of the new node.</returns>
    TNodeId New(TNodeId parentId);

    Node const& Get(TNodeId nodeId) const
    {
        LV_ASSERT(Has(nodeId), "Invalid node ID!");
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

    Storage<Node> m_Nodes;
    std::vector<int> m_Heights;
};

}

#endif // ifndef LV_NTREE_H
