#pragma once

#include "Math/Math.h"
#include "Math/glm.h"


namespace Limnova
{

    class Camera
    {
    public:
        struct Data
        {
            glm::mat4 ViewProj;
            glm::vec3 Position;
        /*--pad 1byte-------------------------*/private: float pad0; public:
            glm::vec3 AimDirection;
        /*--pad 1byte-------------------------*/private: float pad1; public:

            Data(glm::mat4 viewProj, glm::vec3 position, glm::vec3 aimDirection)
                : ViewProj(viewProj), Position(position), AimDirection(aimDirection) {}
        };

        virtual ~Camera() = default;

    protected:
        virtual void RecomputeData() = 0;
    private:
        friend class Renderer;
        friend class Renderer2D;
        virtual Data const* GetData() = 0;
    };

}
