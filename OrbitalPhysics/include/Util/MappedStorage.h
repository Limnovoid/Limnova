#ifndef LV_MAPPED_STORAGE_H
#define LV_MAPPED_STORAGE_H

#include "Storage.h"

#include <unordered_map>

namespace Limnova
{

/// <summary>
/// Mapped storage class. Internally uses a Util::Storage object to create and store items, and maps user-defined keys to the item IDs for retrieval.
/// </summary>
/// <typeparam name="TKey">Type to use as map key.</typeparam>
/// <typeparam name="TItem">Type of item to store.</typeparam>
template<typename TKey, typename TItem>
class MappedStorage
{
public:
	using TId		= Storage<TItem>::TId;

	MappedStorage();

	size_t Size();

	bool Has(TKey const key) const;

	TItem& Add(TKey const key);

	TItem& Get(TKey const key);

	TItem& GetOrAdd(TKey const key);

	void Remove(TKey const key);

	bool TryRemove(TKey const key);

	TItem& operator[](TKey const key);

private:
	using IdMap								= std::unordered_map<TKey, TId>;	/// Type of map of user keys to object IDs.

	Storage<TItem>							m_storage;							/// Object storage.
	IdMap									m_idMap;							/// Map of user keys to object IDs.
};

// ---------------------------------------------------------------------------------------------------------------------------------

template<typename TKey, typename TItem>
MappedStorage<TKey, TItem>::MappedStorage()
{
}

// ---------------------------------------------------------------------------------------------------------------------------------

template<typename TKey, typename TItem>
size_t MappedStorage<TKey, TItem>::Size()
{
	return m_idMap.size();
}

// ---------------------------------------------------------------------------------------------------------------------------------

template<typename TKey, typename TItem>
bool MappedStorage<TKey, TItem>::Has(TKey const key) const
{
	return m_idMap.contains(key);
}

// ---------------------------------------------------------------------------------------------------------------------------------

template<typename TKey, typename TItem>
TItem& MappedStorage<TKey, TItem>::Add(TKey const key)
{
	LV_ASSERT(!Has(key), "This key alread exists.");

	TId attrId = m_storage.New();

	m_idMap.emplace(key, attrId);

	return m_storage[attrId];
}

// ---------------------------------------------------------------------------------------------------------------------------------

template<typename TKey, typename TItem>
TItem& MappedStorage<TKey, TItem>::Get(TKey const key)
{
	LV_ASSERT(Has(key), "Key not found.");

	return m_storage.Get(m_idMap.at(key));
}

// ---------------------------------------------------------------------------------------------------------------------------------

template<typename TKey, typename TItem>
TItem& MappedStorage<TKey, TItem>::GetOrAdd(TKey const key)
{
	return Has(key) ? Get(key) : Add(key);
}

// ---------------------------------------------------------------------------------------------------------------------------------

template<typename TKey, typename TItem>
void MappedStorage<TKey, TItem>::Remove(TKey const key)
{
	LV_ASSERT(Has(key), "Key not found.");

	m_storage.Erase(m_idMap.at(key));
	m_idMap.erase(key);
}

// ---------------------------------------------------------------------------------------------------------------------------------

template<typename TKey, typename TItem>
bool MappedStorage<TKey, TItem>::TryRemove(TKey const key)
{
	if (Has(key))
	{
		Remove(key);
		return true;
	}

	return false;
}

// ---------------------------------------------------------------------------------------------------------------------------------

template<typename TKey, typename TItem>
TItem& MappedStorage<TKey, TItem>::operator[](TKey const key)
{
	LV_ASSERT(Has(key), "Key not found.");

	return Get(key);
}

}

#endif // ifndef LV_MAPPED_STORAGE_H
