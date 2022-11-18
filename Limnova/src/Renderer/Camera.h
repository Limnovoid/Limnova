#pragma once

#include "Math/glm.h"


namespace Limnova
{

    class Camera
    {
    public:
        struct BufferData
        {
            glm::mat4 ViewProj;
            glm::vec4 Position;
            glm::vec4 AimDirection;
        };

        virtual ~Camera() {}

        virtual void RecomputeData() = 0;
        virtual BufferData const* GetData() = 0;

        virtual bool IsActive() = 0;
        virtual void SetActive() = 0;
        virtual void SetNotActive() = 0;
    };

}
