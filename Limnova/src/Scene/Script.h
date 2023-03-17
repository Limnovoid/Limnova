#pragma once

#include "Entity.h"


namespace Limnova
{

    class NativeScript
    {
    public:
        virtual ~NativeScript() {}

        template<typename T>
        T& GetComponent()
        {
            return m_Entity.GetComponent<T>();
        }

        bool IsActiveCamera()
        {
            return m_Entity == m_Entity.m_Scene->GetActiveCamera();
        }
    protected:
        virtual void OnCreate() {}
        virtual void OnDestroy() {}
        virtual void OnUpdate(Timestep) {}
        virtual void OnEvent(Event&) {}
    private:
        Entity m_Entity;

        friend class Scene;
    };

}
