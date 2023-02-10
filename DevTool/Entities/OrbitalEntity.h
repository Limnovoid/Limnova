#pragma once

#include "OrbitSystem2D.h"


class OrbitalEntity
{
public:
    OrbitalEntity() {}

    virtual ~OrbitalEntity() {}
protected:
    OrbitSystem2D::NodeRef m_Node;
};


class OrbitSystemHost : public OrbitalEntity
{
public:
    OrbitSystemHost();
    ~OrbitSystemHost();

};
