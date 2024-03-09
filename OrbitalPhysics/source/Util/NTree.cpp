#include "Util/NTree.h"

#include "Util/Log.h"

namespace Limnova
{

NTree::NTree()
{
}

// ---------------------------------------------------------------------------------------------------------------------------------

NTree::NTree(NTree const& rhs)
{
}

// ---------------------------------------------------------------------------------------------------------------------------------

size_t NTree::Size() const
{
	return m_Nodes.Size();
}

// ---------------------------------------------------------------------------------------------------------------------------------

bool NTree::Has(TNodeId const nodeId) const
{
	return m_Nodes.Has(nodeId);
}

// ---------------------------------------------------------------------------------------------------------------------------------

NTree::TNodeId NTree::New()
{
	TNodeId nodeId = GetEmpty();

	if (nodeId == 0)
		m_Heights[nodeId] = 0;
	else
		Attach(nodeId, 0);

	return nodeId;
}

// ---------------------------------------------------------------------------------------------------------------------------------

NTree::TNodeId NTree::New(TNodeId const parentId)
{
	LV_ASSERT(Has(parentId), "Invalid parent ID!");

	TNodeId nodeId = GetEmpty();
	Attach(nodeId, parentId);
	return nodeId;
}

// ---------------------------------------------------------------------------------------------------------------------------------

NTree::Node const& NTree::Get(TNodeId const nodeId) const
{
	LV_ASSERT(Has(nodeId), "Invalid node ID!");
	return m_Nodes.Get(nodeId);
}

// ---------------------------------------------------------------------------------------------------------------------------------

NTree::THeight NTree::Height(TNodeId const nodeId) const
{
	LV_ASSERT(Has(nodeId), "Invalid node ID!");

	return m_Heights[nodeId];
}

// ---------------------------------------------------------------------------------------------------------------------------------

void NTree::Remove(TNodeId const nodeId)
{
	if (nodeId == 0)
	{
		Clear();
	}
	else
	{
		Detach(nodeId);
		EraseSubtree(nodeId);
	}
}

// ---------------------------------------------------------------------------------------------------------------------------------

void NTree::Clear()
{
	m_Nodes.Clear();
	m_Heights.clear();
}

// ---------------------------------------------------------------------------------------------------------------------------------

void NTree::Move(TNodeId const nodeId, TNodeId const newParentId)
{
	Detach(nodeId);
	Attach(nodeId, newParentId);
}

// ---------------------------------------------------------------------------------------------------------------------------------

void NTree::SwapWithPrevSibling(TNodeId const nodeId)
{
	LV_ASSERT(Has(nodeId), "Invalid node ID!");

	auto& node = m_Nodes[nodeId];

	if (NNull == node.PrevSibling)
	{
		LV_LOG("SwapWithPrevSibling called but node has no previous sibling.");
		return;
	}

	Node& prev = m_Nodes[node.PrevSibling];
	Node& parent = m_Nodes[node.Parent];

	if (parent.FirstChild == node.PrevSibling) parent.FirstChild = nodeId;
	if (prev.PrevSibling != NNull) m_Nodes[prev.PrevSibling].NextSibling = nodeId;
	if (node.NextSibling != NNull) m_Nodes[node.NextSibling].PrevSibling = node.PrevSibling;

	prev.NextSibling = node.NextSibling;
	node.NextSibling = node.PrevSibling;
	node.PrevSibling = prev.PrevSibling;
	prev.PrevSibling = nodeId;
}

// ---------------------------------------------------------------------------------------------------------------------------------

void NTree::SwapWithNextSibling(TNodeId const nodeId)
{
	LV_ASSERT(Has(nodeId), "Invalid node ID!");

	Node& node = m_Nodes[nodeId];

	if (NNull == node.NextSibling)
	{
		LV_LOG("SwapWithNextSibling called but node has no next sibling.");
		return;
	}

	Node& next = m_Nodes[node.NextSibling];
	Node& parent = m_Nodes[node.Parent];

	if (parent.FirstChild == nodeId) parent.FirstChild = node.NextSibling;
	if (next.NextSibling != NNull) m_Nodes[next.NextSibling].PrevSibling = nodeId;
	if (node.PrevSibling != NNull) m_Nodes[node.PrevSibling].NextSibling = node.NextSibling;

	next.PrevSibling = node.PrevSibling;
	node.PrevSibling = node.NextSibling;
	node.NextSibling = next.NextSibling;
	next.NextSibling = nodeId;
}

// ---------------------------------------------------------------------------------------------------------------------------------

size_t NTree::GetChildren(TNodeId const nodeId, std::vector<TNodeId>& children) const
{
	LV_ASSERT(Has(nodeId), "Invalid node ID!");

	size_t numChildren = 0;

	TNodeId child = m_Nodes[nodeId].FirstChild;
	while (child != NNull)
	{
		numChildren++;
		children.push_back(child);
		child = m_Nodes[child].NextSibling;
	}
	return numChildren;
}

// ---------------------------------------------------------------------------------------------------------------------------------

size_t NTree::GetSubtree(TNodeId const rootNodeId, std::vector<TNodeId>& subtree) const
{
	size_t numAdded = GetChildren(rootNodeId, subtree);

	size_t sizeSubtree = numAdded;
	do
	{
		size_t end = subtree.size();
		size_t idx = end - numAdded;
		numAdded = 0;

		while (idx < end)
		{
			numAdded += GetChildren(subtree[idx], subtree);
			idx++;
		}

		sizeSubtree += numAdded;
	}
	while (numAdded > 0);

	return sizeSubtree;
}

// ---------------------------------------------------------------------------------------------------------------------------------

NTree::TNodeId NTree::GetParent(TNodeId const nodeId) const
{
	LV_ASSERT(Has(nodeId), "Invalid node ID!");

	if (0 == nodeId)
	{
		LV_LOG("GetParent called but node does not have a parent.");
		return NNull;
	}

	return m_Nodes[nodeId].Parent;
}

// ---------------------------------------------------------------------------------------------------------------------------------

NTree::TNodeId NTree::GetGrandparent(TNodeId const nodeId) const
{
	LV_ASSERT(Has(nodeId), "Invalid node ID!");

	if (1 > m_Heights[nodeId])
	{
		LV_LOG("GetGrandparent called but node does not have a grandparent.");
		return NNull;
	}

	return m_Nodes[m_Nodes[nodeId].Parent].Parent;
}

// ---------------------------------------------------------------------------------------------------------------------------------

NTree::Node const& NTree::operator[](TNodeId nodeId) const
{
	LV_ASSERT(Has(nodeId), "Invalid node ID!");

	return Get(nodeId);
}

// ---------------------------------------------------------------------------------------------------------------------------------

NTree::TNodeId NTree::GetEmpty()
{
	TNodeId emptyId = m_Nodes.New();

	if (emptyId >= m_Heights.size())
		m_Heights.push_back(~THeight(1));

	return emptyId;
}

// ---------------------------------------------------------------------------------------------------------------------------------

void NTree::EraseSubtree(TNodeId const rootId)
{
	LV_ASSERT(Has(rootId), "Invalid node ID!");

	TNodeId childId = m_Nodes[rootId].FirstChild;
	while (NNull != childId)
	{
		TNodeId nextSiblingId = m_Nodes[childId].NextSibling;
		EraseSubtree(childId);
		childId = nextSiblingId;
	}

	m_Nodes.Erase(rootId);
}

// ---------------------------------------------------------------------------------------------------------------------------------

void NTree::Attach(TNodeId const nodeId, TNodeId const parentId)
{
	Node& node = m_Nodes[nodeId];
	Node& parent = m_Nodes[parentId];

	node.Parent = parentId;

	if (parent.FirstChild != NNull)
	{
		// Replace parent's first child
		node.NextSibling = parent.FirstChild;
		m_Nodes[parent.FirstChild].PrevSibling = nodeId;
	}

	parent.FirstChild = nodeId;

	m_Heights[nodeId] = m_Heights[parentId] + 1;
}

// ---------------------------------------------------------------------------------------------------------------------------------

void NTree::Detach(TNodeId nodeId)
{
	Node& node = m_Nodes[nodeId];

	if (NNull == node.Parent)
	{
		LV_LOG("Detach called but node ({}) is not attached to the tree.", nodeId);
		return;
	}

	Node& parent = m_Nodes[node.Parent];

	if (parent.FirstChild == nodeId)
		parent.FirstChild = node.NextSibling;

	node.Parent = NNull;

	// Connect siblings to eachother
	if (node.NextSibling != NNull) m_Nodes[node.NextSibling].PrevSibling = node.PrevSibling;
	if (node.PrevSibling != NNull) m_Nodes[node.PrevSibling].NextSibling = node.NextSibling;

	node.NextSibling = node.PrevSibling = NNull;

	m_Heights[nodeId] = ~THeight(0);
}

}
