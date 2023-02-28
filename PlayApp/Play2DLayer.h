#pragma once

#include <Limnova.h>
#include <Core/Layer.h>


class LIMNOVA_API Play2DLayer : public Limnova::Layer
{
public:
    Play2DLayer();
    virtual ~Play2DLayer() = default;

    void OnAttach() override;
    void OnDetach() override;

    void OnUpdate(Limnova::Timestep dT) override;
    void OnImGuiRender() override;
    void OnEvent(Limnova::Event& e) override;
private:
    Limnova::Ref<Limnova::PerspectivePlanarCameraController> m_CameraController;
    //Limnova::Ref<Limnova::OrthographicPlanarCameraController> m_CameraController;

    Limnova::ShaderLibrary m_ShaderLibrary;
    Limnova::Ref<Limnova::Texture2D> m_TurretTexture;
    Limnova::Ref<Limnova::Texture2D> m_CheckerboardTexture;
    Limnova::Ref<Limnova::VertexArray> m_SquareVA;
    Limnova::Ref<Limnova::Texture2D> m_SpriteSheet;
    Limnova::Ref<Limnova::SubTexture2D> m_SpriteStairs, m_SpriteTree;

    Limnova::Vector4 m_SquareColor = { 0.2f, 0.3f, 0.9f, 1.f };
    Limnova::Vector4 m_TextureTint = { 0.2f, 0.2f, 0.2f, 1.f };
    Limnova::Vector2 m_TextureScale = { 3.f, 3.f };
    float m_BackgroundRotation = 0.f;
};
