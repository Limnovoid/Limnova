#include "Entity.h"


namespace Limnova
{

    void Entity::Destroy()
    {
        m_Scene->m_Registry.destroy(m_EnttId);
    }

}
