#pragma once

#include "Core.h"


namespace Limnova
{

    class LIMNOVA_API Input
    {
    public:
        static bool IsKeyPressed(int keycode);

        static bool IsMouseButtonPressed(int button);
        static std::pair<float, float> GetMousePosition();
    };

}