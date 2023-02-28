#pragma once

#include <Limnova.h>

namespace LV = Limnova;


class TestLayer : public Limnova::Layer
{
public:
    TestLayer();

    void OnAttach();
    void OnDetach();
    void OnUpdate(LV::Timestep dT);
    void OnImGuiRender();
    void OnEvent(LV::Event& e);
public:
    bool Test_BigFloatComparisonOperators();
};
