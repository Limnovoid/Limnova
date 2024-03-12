#pragma once

#include "Texture.h"
#include "Math/LimnovaMath.h"


namespace Limnova
{

    class SubTexture2D
    {
    public:
        SubTexture2D(const Ref<Texture2D>& texture, const Vector2& min, const Vector2& max);

        const Ref<Texture2D> GetTexture() { return m_Texture; }
        const Vector2* GetTexCoords() { return m_TexCoords; }

        static Ref<SubTexture2D> CreateFromCoords(const Ref<Texture2D>& texture, const Vector2& coords, const Vector2& cellSize, const Vector2& spriteSize = { 1, 1 });
    private:
        Ref<Texture2D> m_Texture;
        Vector2 m_TexCoords[4];
    };

}
