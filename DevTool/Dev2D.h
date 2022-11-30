#pragma once

#include <Limnova.h>
#include <Core/Layer.h>


class LIMNOVA_API Dev2DLayer : public Limnova::Layer
{
public:
    Dev2DLayer();
    virtual ~Dev2DLayer() = default;

    void OnAttach() override;
    void OnDetach() override;

    void OnUpdate(Limnova::Timestep dT) override;
    void OnImGuiRender() override;
    void OnEvent(Limnova::Event& e) override;
private:
    Limnova::Ref<Limnova::OrthographicPointCameraController> m_CameraController;

    // TEMPORARY for future abstraction into 2D renderer
    Limnova::ShaderLibrary m_ShaderLibrary;
    Limnova::Ref<Limnova::Texture2D> m_Texture;
    Limnova::Ref<Limnova::VertexArray> m_SquareVA;

    glm::vec4 m_SquareColor = { 0.2f, 0.3f, 0.9f, 1.f };
};
