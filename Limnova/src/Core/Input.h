#pragma once

#include "Core.h"
#include "KeyCodes.h"
#include "MouseButtonCodes.h"


namespace Limnova
{

#define LV_ASCII_PRINTABLE_MIN 32
#define LV_ASCII_0 48
#define LV_ASCII_A 65
#define LV_ASCII_a 97
#define LV_ASCII_PRINTABLE_MAX 127

    class LIMNOVA_API Input
    {
    public:
        static bool IsKeyPressed(KeyCode keycode);

        static bool IsMouseButtonPressed(MouseButton button);
        static std::pair<float, float> GetMousePosition();
    };

}