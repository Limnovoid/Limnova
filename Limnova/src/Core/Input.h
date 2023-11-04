#pragma once

#include "Core.h"
#include "KeyCodes.h"
#include "MouseButtonCodes.h"


namespace Limnova
{

    class LIMNOVA_API Input
    {
    public:
        static bool IsKeyPressed(KeyCode keycode);

        static bool IsMouseButtonPressed(MouseButton button);
        static std::pair<float, float> GetMousePosition();
    };

}