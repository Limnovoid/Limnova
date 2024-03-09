#ifndef LV_STORAGE_H
#define LV_STORAGE_H

#include "Log.h"

#include <vector>
#include <unordered_set>

namespace Limnova
{

/// <summary> Dynamic array-based storage class intended for efficient re-use of allocated memory. </summary>
/// <typeparam name="T">Type of item to store.</typeparam>
template<typename T>
class Storage
{
public:
	using TId						= uint32_t;								/// Type of item ID.

	static constexpr TId IdNull		= ~TId(0);								/// The Null ID.

	Storage();

	/// <summary> Get number of in-use items (total storage minus recycled items). </summary>
	size_t Size() const;

	/// <summary> Check if ID is of an item currently in-use (item is allocated and not recycled). </summary>
	bool Has(TId const id) const;

	/// <summary> Get a new item. Re-uses ID of a previously allocated item or, if none of those exists, constructs new. </summary>
	/// <returns>ID of new item.</returns>
	TId New();

	/// <summary> Get a new item. Re-uses ID of a previously allocated item or, if none of those exists, constructs new. </summary>
	/// <param name="pItem">Storage for a pointer to the new item.</param>
	/// <returns>ID of new item.</returns>
	TId New(T*& pItem);

	/// <summary> Get reference to stored item. </summary>
	T& Get(TId id);

	/// <summary> Get item by const reference. </summary>
	T const& Get(TId const id) const;

	/// <summary> Reset the item to default state and recycle it for future use (keeps item in internal memory and stores ID for future distribution). </summary>
	/// <param name="id">ID of item to erase.</param>
	void Erase(TId const id);

	/// <summary> If an item with the given ID exists, erase it. </summary>
	/// <param name="id">ID of item to erase.</param>
	/// <returns>True if item was erased, false if no item with ID was found.</returns>
	bool TryErase(TId const id);

	/// <summary> Clear all internal storage. </summary>
	void Clear();

	T& operator[](TId const id);
	T const& operator[](TId const id) const;

private:
	using ItemList					= std::vector<T>;						/// Type of item storage.
	using EmptiesSet				= std::unordered_set<TId>;				/// Type of set of empty items' IDs.

	/// <summary> Get ID of an unused allocated item or, if none of those exists, the ID of a newly constructed item. </summary>
	/// <returns>ID of item in internal storage.</returns>
	TId GetEmpty();

	/// <summary> Get ID of an unused allocated item or, if none of those exists, the ID of a newly constructed item. </summary>
	/// <param name="pItem">Storage for a pointer to the empty item.</param>
	/// <returns>ID of item in internal storage.</returns>
	TId GetEmpty(T*& pItem);

	/// <summary> Reset the item with the given ID to default its state (assign default constructor values) and save the ID for future re-use. </summary>
	/// <param name="id">ID of item to recycle.</param>
	void Recycle(TId const id);

	ItemList						m_items;								/// Item storage.
	EmptiesSet						m_empties;								/// Set of empty items' IDs.
};

// ---------------------------------------------------------------------------------------------------------------------------------

template<typename T>
Storage<T>::Storage()
{
}

// ---------------------------------------------------------------------------------------------------------------------------------

template<typename T>
size_t Storage<T>::Size() const
{
	return m_items.size() - m_empties.size();
}

// ---------------------------------------------------------------------------------------------------------------------------------

template<typename T>
bool Storage<T>::Has(TId const id) const
{
	return ((m_items.size() > id) && !m_empties.contains(id));
}

// ---------------------------------------------------------------------------------------------------------------------------------

template<typename T>
Storage<T>::TId Storage<T>::New()
{
	return GetEmpty();
}

// ---------------------------------------------------------------------------------------------------------------------------------

template<typename T>
Storage<T>::TId Storage<T>::New(T*& pItem)
{
	return GetEmpty(pItem);
}

// ---------------------------------------------------------------------------------------------------------------------------------

template<typename T>
T& Storage<T>::Get(TId const id)
{
	LV_ASSERT(Has(id), "Invalid ID.");

	return m_items[id];
}

// ---------------------------------------------------------------------------------------------------------------------------------

template<typename T>
T const& Storage<T>::Get(TId const id) const
{
	LV_ASSERT(Has(id), "Invalid ID.");

	return m_items[id];
}

// ---------------------------------------------------------------------------------------------------------------------------------

template<typename T>
void Storage<T>::Erase(TId const id)
{
	LV_ASSERT(Has(id), "Invalid ID!");

	Recycle(id);
}

// ---------------------------------------------------------------------------------------------------------------------------------

template<typename T>
bool Storage<T>::TryErase(TId const id)
{
	if (Has(id))
	{
		Recycle(m_items[id]);
		return true;
	}

	return false;
}

// ---------------------------------------------------------------------------------------------------------------------------------

template<typename T>
void Storage<T>::Clear()
{
	m_items.clear();
	m_empties.clear();
}

// ---------------------------------------------------------------------------------------------------------------------------------

template<typename T>
T& Storage<T>::operator[](TId const id)
{
	LV_ASSERT(Has(id), "Invalid ID.");

	return m_items[id];
}

// ---------------------------------------------------------------------------------------------------------------------------------

template<typename T>
T const& Storage<T>::operator[](TId const id) const
{
	LV_ASSERT(Has(id), "Invalid ID.");
	return m_items[id];
}

// ---------------------------------------------------------------------------------------------------------------------------------

template<typename T>
Storage<T>::TId Storage<T>::GetEmpty()
{
	TId emptyId;

	if (m_empties.empty())
	{
		emptyId = m_items.size();
		m_items.emplace_back();
	}
	else
	{
		EmptiesSet::const_iterator it = m_empties.begin();
		emptyId = *it;
		m_empties.erase(it);
	}

	return emptyId;
}

template<typename T>
Storage<T>::TId Storage<T>::GetEmpty(T*& pItem)
{
	TId emptyId;

	if (m_empties.empty())
	{
		emptyId = m_items.size();
		pItem = &(m_items.emplace_back());
	}
	else
	{
		EmptiesSet::const_iterator it = m_empties.begin();

		emptyId = *it;
		pItem = &(m_items[emptyId]);

		m_empties.erase(it);
	}

	return emptyId;
}

// ---------------------------------------------------------------------------------------------------------------------------------

template<typename T>
void Storage<T>::Recycle(TId const id)
{
	m_items[id] = T();
	m_empties.insert(id);
}

}

#endif // ifndef LV_STORAGE_H
