#include "Entity.h"


uint32_t Entity::m_IdTop = 0;
std::unordered_set<uint32_t> Entity::m_ReusableIds = {};

uint32_t Entity::ReserveId()
{
    if (!m_ReusableIds.empty())
    {
        auto it = m_ReusableIds.begin();
        auto id = *it;
        m_ReusableIds.erase(it);
        return id;
    }
    return m_IdTop++;
}


void Entity::ReleaseId(const uint32_t id)
{
    m_ReusableIds.insert(id);
}
