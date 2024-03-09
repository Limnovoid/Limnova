#ifndef LV_NTREE_H
#define LV_NTREE_H

#include "Storage.h"

#include <vector>
#include <limits>

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
	using TNodeId					= uint32_t;								/// Type of tree node ID;
	using THeight					= uint32_t;								/// Type of tree node height.

	static constexpr TNodeId NNull	= std::numeric_limits<TNodeId>::max();	/// The Null node ID;

	/// <summary> Node class from which the NTree is constructed - its members are Node IDs describing its relationships with adjacent Nodes. </summary>
	struct Node
	{
		TNodeId Parent				= NNull;
		TNodeId NextSibling			= NNull;
		TNodeId PrevSibling			= NNull;
		TNodeId FirstChild			= NNull;
	};

	NTree();

	NTree(NTree const& rhs);

	/// <summary> Get the number of nodes in the tree. </summary>
	size_t Size() const;

	/// <summary> Check if the given ID corresponds to a node in the tree. </summary>
	/// <returns>True if there is a node in the tree with the given ID, otherwise false.</returns>
	bool Has(TNodeId const nodeId) const;

	/// <summary>
	/// Adds a new node to the tree. If the tree is empty, the new node is the root node, otherwise the new node is parented to the root node.
	/// Re-uses a previously allocated Node object or, if none exists, constructs new.
	/// </summary>
	/// <returns>ID of the new node.</returns>
	TNodeId New();

	/// <summary>
	/// Adds a new node to the tree parented to a given node. Cannot be called on an empty tree.
	/// Re-uses a previously allocated Node object or, if none exists, constructs new.
	/// </summary>
	/// <param name="nodeId">The ID of the parent node to attach the new node to.</param>
	/// <returns>ID of the new node.</returns>
	TNodeId New(TNodeId const parentId);

	/// <summary> Get a const reference to a node. </summary>
	/// <param name="nodeId">The ID of the node.</param>
	Node const& Get(TNodeId const nodeId) const;

	/// <summary> Get the height of a node. </summary>
	/// <param name="nodeId">The ID of the node.</param>
	THeight Height(TNodeId const nodeId) const;

	/// <summary>
	/// Remove a node from the tree. Does not deallocate the node object - reserves it for subsequent calls to New.
	/// Also removes all children (grandchildren, etc) attached to this node.
	/// </summary>
	/// <param name="nodeId">The ID of the node.</param>
	void Remove(TNodeId const nodeId);

	/// <summary> Clear the tree. Deallocates all used and unused node objects. </summary>
	void Clear();

	/// <summary> Move a node to a new parent (a.k.a. reparent, reattach, etc). </summary>
	/// <param name="nodeId">The node to move.</param>
	/// <param name="newParentId">The parent of the node after the move.</param>
	void Move(TNodeId const nodeId, TNodeId const newParentId);

	/// <summary>
	/// Swap a node with its previous sibling in its ordered list of siblings.
	/// If the previous sibling is the parent's first child, this node becomes the parent's first child.
	/// </summary>
	/// <param name="nodeId">The ID of the node.</param>
	void SwapWithPrevSibling(TNodeId const nodeId);

	/// <summary>
	/// Swap a node with its next sibling in its ordered list of siblings.
	/// If this node is the parent's first child, the next sibling becomes the parent's first child.
	/// </summary>
	/// <param name="nodeId">The ID of the node.</param>
	void SwapWithNextSibling(TNodeId const nodeId);

	/// <summary>
	/// Get a node's children as an ordered list of IDs.
	/// The list is added to the back of the given vector and ordered according to the childen's own sibling list:
	/// first the parent node's first child is added to the back of the vector, followed by that child's next sibling, and so on.
	/// The given parent node is not included in the final list.
	/// </summary>
	/// <param name="nodeId">The ID of the parent node.</param>
	/// <param name="children">Storage for the list of children.</param>
	/// <returns>The number of children added to the vector.</returns>
	size_t GetChildren(TNodeId const nodeId, std::vector<TNodeId>& children) const;

	/// <summary>
	/// Get a node's subtree as an ordered list of IDs.
	/// The list is added to the back of the given vector.
	/// It is ordered first by height, so all nodes on the same level of the subtree appear contiguously;
	/// each level is then ordered by parent, so all nodes with the same parent appear contiguously and these groups of sibling nodes appear in the order of their parent's sibling list;
	/// finally each group of sibling nodes is ordered according to its sibling list.
	/// The given root node is not included in the final list.
	/// </summary>
	/// <param name="rootNodeId">The ID of the subtree's root node.</param>
	/// <param name="subtree">Storage for the list of subtree nodes.</param>
	/// <returns>The total number of nodes added to the vector.</returns>
	size_t GetSubtree(TNodeId const rootNodeId, std::vector<TNodeId>& subtree) const;

	/// <summary> Get a node's parent. Returns the Null ID if the given node has no parent - is the root node. </summary>
	/// <param name="nodeId">The node's ID.</param>
	/// <returns>ID of the parent, or Null if has no parent.</returns>
	TNodeId GetParent(TNodeId const nodeId) const;

	/// <summary> Get a node's grandparent. Returns the Null ID if the given node has no grandparent - is or is parented to the root node. </summary>
	/// <param name="nodeId">The node's ID.</param>
	/// <returns>ID of the grandparent, or Null if has no grandparent.</returns>
	TNodeId GetGrandparent(TNodeId const nodeId) const;

	/// <summary> Get a const reference to a node. </summary>
	/// <param name="nodeId">The ID of the node.</param>
	Node const& operator[](TNodeId nodeId) const;

private:
	/// <summary> Get ID of an empty node (not attached to the tree and has no children) from internal node storage. </summary>
	TNodeId GetEmpty();

	/// <summary> Erase all nodes in a subtree, including the root. </summary>
	/// <param name="rootId">The ID of the subtree's root node.</param>
	void EraseSubtree(TNodeId const rootId);

	/// <summary>
	/// Attach a node to a parent node. Node replaces the parent's first child, if one exists.
	/// Does not detach the node if it is already attached - use only on an empty node, or call Detach before calling this function.
	/// </summary>
	/// <param name="nodeId">The ID of the node to attach.</param>
	/// <param name="parentId">The ID of the node which will receive the new child node.</param>
	void Attach(TNodeId const nodeId, TNodeId const parentId);

	/// <summary>
	/// Detach a node from the tree.
	/// All nodes attached to this node stay attached to it - i.e, this function detaches the subtree rooted in the specified node, and the subtree remains intact.
	/// </summary>
	/// <param name="nodeId"></param>
	void Detach(TNodeId nodeId);

	Storage<Node>					m_Nodes;								/// The node storage.
	std::vector<THeight>			m_Heights;								/// The node heights.
};

}

#endif // ifndef LV_NTREE_H
