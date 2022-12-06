#pragma once

#include <Limnova.h>


class Orbiter
{
public:
    Orbiter();
    Orbiter(Limnova::Vector2 position, Limnova::Vector2 scale);
    ~Orbiter();

    Limnova::Vector2 m_Position;
    Limnova::Vector2 m_Scale;
};
