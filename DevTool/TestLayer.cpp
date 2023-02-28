#include "TestLayer.h"

namespace LV = Limnova;


TestLayer::TestLayer()
    : Layer("TestLayer")
{
}


void TestLayer::OnAttach()
{
    if (Test_BigFloatComparisonOperators()) LV_INFO("Test set passed: BigFloat Comparison Operators!");
}


void TestLayer::OnDetach() {}
void TestLayer::OnUpdate(LV::Timestep dT) {}
void TestLayer::OnImGuiRender() {}
void TestLayer::OnEvent(LV::Event& e) {}


bool TestLayer::Test_BigFloatComparisonOperators()
{
    // Positive non-zero > zero = true
    LV_ASSERT(LV::BigFloat(1.f, 10) > LV::BigFloat::Zero, "");
    LV_ASSERT(LV::BigFloat(1.f, 0) > LV::BigFloat::Zero, "");
    LV_ASSERT(LV::BigFloat(1.f, -10) > LV::BigFloat::Zero, "");
    // Positive non-zero < zero = false
    LV_ASSERT(!(LV::BigFloat(1.f, 10) < LV::BigFloat::Zero), "");
    LV_ASSERT(!(LV::BigFloat(1.f, 0) < LV::BigFloat::Zero), "");
    LV_ASSERT(!(LV::BigFloat(1.f, -10) < LV::BigFloat::Zero), "");
    // Negative non-zero < zero = true
    LV_ASSERT(LV::BigFloat(-1.f, 10) < LV::BigFloat::Zero, "");
    LV_ASSERT(LV::BigFloat(-1.f, 0) < LV::BigFloat::Zero, "");
    LV_ASSERT(LV::BigFloat(-1.f, -10) < LV::BigFloat::Zero, "");
    // Negative non-zero > zero = false
    LV_ASSERT(!(LV::BigFloat(-1.f, 10) > LV::BigFloat::Zero), "");
    LV_ASSERT(!(LV::BigFloat(-1.f, 0) > LV::BigFloat::Zero), "");
    LV_ASSERT(!(LV::BigFloat(-1.f, -10) > LV::BigFloat::Zero), "");
    // Negative < positive = true
    LV_ASSERT(LV::BigFloat(-1.f, -10) < LV::BigFloat(1.f, -10), "");
    LV_ASSERT(LV::BigFloat(-1.f, 0) < LV::BigFloat(1.f, 1), "");
    LV_ASSERT(LV::BigFloat(-1.f, 10) < LV::BigFloat(1.f, 10), "");
    // Negative > positive = false
    LV_ASSERT(!(LV::BigFloat(-1.f, 10) > LV::BigFloat(1.f, -10)), "");
    LV_ASSERT(!(LV::BigFloat(-1.f, 0) > LV::BigFloat(1.f, 1)), "");
    LV_ASSERT(!(LV::BigFloat(-1.f, -10) > LV::BigFloat(1.f, 10)), "");
    // Positive > negative = true
    LV_ASSERT(LV::BigFloat(1.f, -10) > LV::BigFloat(-1.f, -10), "");
    LV_ASSERT(LV::BigFloat(1.f, 0) > LV::BigFloat(-1.f, 1), "");
    LV_ASSERT(LV::BigFloat(1.f, 10) > LV::BigFloat(-1.f, 10), "");
    // Positive < negative = false
    LV_ASSERT(!(LV::BigFloat(1.f, -10) < LV::BigFloat(-1.f, -10)), "");
    LV_ASSERT(!(LV::BigFloat(1.f, 0) < LV::BigFloat(-1.f, 1)), "");
    LV_ASSERT(!(LV::BigFloat(1.f, 10) < LV::BigFloat(-1.f, 10)), "");
    // Value > value = false
    LV_ASSERT(!(LV::BigFloat(1.f, -10) > LV::BigFloat(1.f, -10)), "");
    LV_ASSERT(!(LV::BigFloat(1.f, 0) > LV::BigFloat(1.f, 0)), "");
    LV_ASSERT(!(LV::BigFloat(1.f, 10) > LV::BigFloat(1.f, 10)), "");
    LV_ASSERT(!(LV::BigFloat(-1.f, -10) > LV::BigFloat(-1.f, -10)), "");
    LV_ASSERT(!(LV::BigFloat(-1.f, 0) > LV::BigFloat(-1.f, 0)), "");
    LV_ASSERT(!(LV::BigFloat(-1.f, 10) > LV::BigFloat(-1.f, 10)), "");
    // Value < value = false
    LV_ASSERT(!(LV::BigFloat(1.f, -10) < LV::BigFloat(1.f, -10)), "");
    LV_ASSERT(!(LV::BigFloat(1.f, 0) < LV::BigFloat(1.f, 0)), "");
    LV_ASSERT(!(LV::BigFloat(1.f, 10) < LV::BigFloat(1.f, 10)), "");
    LV_ASSERT(!(LV::BigFloat(-1.f, -10) < LV::BigFloat(-1.f, -10)), "");
    LV_ASSERT(!(LV::BigFloat(-1.f, 0) < LV::BigFloat(-1.f, 0)), "");
    LV_ASSERT(!(LV::BigFloat(-1.f, 10) < LV::BigFloat(-1.f, 10)), "");

    return true;
}
