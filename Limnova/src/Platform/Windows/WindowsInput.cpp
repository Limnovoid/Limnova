#include "Core/Input.h"

#include "Core/Application.h"

#include <GLFW/glfw3.h>


namespace Limnova
{

    bool Input::IsKeyPressed(KeyCode keycode)
    {
        auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
        auto state = glfwGetKey(window, (int)keycode);

        return state == GLFW_PRESS || state == GLFW_REPEAT;
    }


    bool Input::IsMouseButtonPressed(MouseButton button)
    {
        auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
        auto state = glfwGetMouseButton(window, (int)button);

        return state == GLFW_PRESS;
    }


    std::pair<float, float> Input::GetMousePosition()
    {
        auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
        double x, y;
        glfwGetCursorPos(window, &x, &y);

        return { (float)x, (float)y };
    }

}