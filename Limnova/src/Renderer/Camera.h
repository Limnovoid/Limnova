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
//private: float pad0; public:
            glm::vec4 AimDirection;
//private: float pad1; public:

            BufferData(glm::mat4 viewProj, glm::vec4 position, glm::vec4 aimDirection)
                : ViewProj(viewProj), Position(position), AimDirection(aimDirection)
            {
            }
        };

        virtual ~Camera() {}

        virtual void RecomputeData() = 0;
        virtual BufferData const* GetData() = 0;
    };

}
