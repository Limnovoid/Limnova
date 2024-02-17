#ifndef LV_STORAGE_H
#define LV_STORAGE_H

namespace Limnova
{


/// <summary> Dynamic array-based storage class intended for efficient re-use of allocated memory. </summary>
/// <typeparam name="T">Type of item to store.</typeparam>
template<typename T>
class Storage
{
public:
    using TId = uint32_t;

    static constexpr TId IdNull = ::std::numeric_limits<TId>::max();

    Storage() = default;
    Storage(const Storage&) = default;

    /// <summary> Get number of in-use items (total storage minus recycled items). </summary>
    size_t Size() const;

    /// <summary> Check if ID is of an item currently in-use (item is allocated and not recycled). </summary>
    bool Has(TId id) const;

    /// <summary> Get a new item. Re-uses ID of a previously allocated item or, if none of those exists, constructs new. </summary>
    /// <returns>ID of new item.</returns>
    TId New();

    /// <summary> Get reference to stored item. </summary>
    T& Get(TId id);

    /// <summary> Get item by const reference. </summary>
    T const& Get(TId id) const;

    /// <summary> Reset the item to default state and recycle it for future use (keeps item in internal memory and stores ID for future distribution). </summary>
    /// <param name="id">ID of item to erase.</param>
    void Erase(TId id);

    /// <summary> If an item with the given ID exists, erase it. </summary>
    /// <param name="id">ID of item to erase.</param>
    /// <returns>True if item was erased, false if no item with ID was found.</returns>
    bool TryErase(TId id);

    /// <summary> Clear all internal storage. </summary>
    void Clear();

    T& operator[](TId id);
    T const& operator[](TId id) const;

private:
    /// <summary> Get ID of an unused allocated item or, if none of those exists, the ID of a newly constructed item. </summary>
    /// <returns>ID of item in internal storage.</returns>
    TId GetEmpty();

    /// <summary> Reset the item with the given ID to default its state (assign default constructor values) and save the ID for future re-use. </summary>
    /// <param name="id">ID of item to recycle.</param>
    void Recycle(TId id);

    std::vector<T> m_Items;
    std::unordered_set<TId> m_Empties;
};

// ---------------------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------------------

template<typename T>
inline size_t Storage<T>::Size() const
{
    return m_Items.size() - m_Empties.size();
}

// ---------------------------------------------------------------------------------------------------------------------------------

template<typename T>
inline bool Storage<T>::Has(TId id) const
{
    return id < m_Items.size() && !m_Empties.contains(id);
}

// ---------------------------------------------------------------------------------------------------------------------------------

template<typename T>
inline Storage<T>::TId Storage<T>::New()
{
    return GetEmpty();
}

// ---------------------------------------------------------------------------------------------------------------------------------

template<typename T>
inline T& Storage<T>::Get(TId id)
{
    LV_CORE_ASSERT(Has(id), "Invalid ID!");
    return m_Items[id];
}

// ---------------------------------------------------------------------------------------------------------------------------------

template<typename T>
inline T const& Storage<T>::Get(TId id) const
{
    LV_CORE_ASSERT(Has(id), "Invalid ID!");
    return m_Items[id];
}

// ---------------------------------------------------------------------------------------------------------------------------------

template<typename T>
inline void Storage<T>::Erase(TId id)
{
    LV_CORE_ASSERT(Has(id), "Invalid ID!");
    Recycle(id);
}

// ---------------------------------------------------------------------------------------------------------------------------------

template<typename T>
inline bool Storage<T>::TryErase(TId id)
{
    if (Has(id)) {
        Recycle(m_Items[id]);
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------------------------------------------------------------

template<typename T>
inline void Storage<T>::Clear()
{
    m_Items.clear();
    m_Empties.clear();
}

// ---------------------------------------------------------------------------------------------------------------------------------

template<typename T>
inline T& Storage<T>::operator[](TId id)
{
    LV_CORE_ASSERT(Has(id), "Invalid ID!");
    return m_Items[id];
}

// ---------------------------------------------------------------------------------------------------------------------------------

template<typename T>
inline T const& Storage<T>::operator[](TId id) const
{
    LV_CORE_ASSERT(Has(id), "Invalid ID!");
    return m_Items[id];
}

// ---------------------------------------------------------------------------------------------------------------------------------

template<typename T>
inline Storage<T>::TId Storage<T>::GetEmpty()
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

// ---------------------------------------------------------------------------------------------------------------------------------

template<typename T>
inline void Storage<T>::Recycle(TId id)
{
    m_Items[id] = T();
    m_Empties.insert(id);
}

}

#endif // ifndef LV_STORAGE_H
