#pragma once

#include <Math/Math.h>


namespace Limnova
{

    struct TagComponent
    {
        std::string Tag;

        TagComponent() = default;
        TagComponent(const TagComponent&) = default;
        TagComponent(const std::string& tag)
            : Tag(tag) {}
    };


    struct TransformComponent
    {
        glm::mat4 Transform{1.f};

        TransformComponent() = default;
        TransformComponent(const TransformComponent&) = default;
        TransformComponent(const glm::mat4& transform)
            : Transform(transform) {}


        operator glm::mat4& () { return Transform; }
        operator const glm::mat4& () const { return Transform; }
    };


    struct SpriteRendererComponent
    {
        Vector4 Color{1.f, 0.f, 1.f, 1.f};

        SpriteRendererComponent() = default;
        SpriteRendererComponent(const SpriteRendererComponent&) = default;
        SpriteRendererComponent(const Vector4& color)
            : Color(color) {}
    };

}

