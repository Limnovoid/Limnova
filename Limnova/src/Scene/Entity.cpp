#include "Entity.h"


namespace Limnova
{

    Entity::Entity(entt::entity id, Scene* scene)
        : m_EnttId(id), m_Scene(scene)
    {
    }

}
