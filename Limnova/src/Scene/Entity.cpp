#include "Entity.h"


namespace Limnova
{

    Entity Entity::Null = Entity();


    void Entity::Destroy()
    {
        m_Scene->m_Registry.destroy(m_EnttId);
    }

}
