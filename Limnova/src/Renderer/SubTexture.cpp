#include "SubTexture.h"


namespace Limnova
{

    SubTexture2D::SubTexture2D(const Ref<Texture2D>& texture, const Vector2& min, const Vector2& max)
        : m_Texture(texture)
    {
        m_TexCoords[0] = { min.x, min.y };
        m_TexCoords[1] = { max.x, min.y };
        m_TexCoords[2] = { max.x, max.y };
        m_TexCoords[3] = { min.x, max.y };
    }


    Ref<SubTexture2D> SubTexture2D::CreateFromCoords(const Ref<Texture2D>& texture, const Vector2& coords, const Vector2& cellSize, const Vector2& spriteSize)
    {
        float xParameter = cellSize.x / texture->GetWidth(), yParameter = cellSize.y / texture->GetHeight();

        Vector2 min{ coords.x * xParameter, coords.y * yParameter };
        Vector2 max{ (coords.x + spriteSize.x) * xParameter, (coords.y + spriteSize.y) * yParameter };

        return CreateRef<SubTexture2D>(texture, min, max);
    }

}

