#pragma once

#include <Limnova.h>


namespace Limnova
{

    class LIMNOVA_API Play2DLayer : public Layer
    {
    public:
        Play2DLayer();
        ~Play2DLayer() = default;

        void OnAttach() override;
        void OnDetach() override;

        void OnUpdate(Timestep dT) override;
        void OnImGuiRender() override;
        void OnEvent(Event& e) override;
    private:
        Ref<Scene> m_Scene;

        Ref<Framebuffer> m_Framebuffer;
        Ref<PerspectivePlanarCameraController> m_CameraController;
        //Ref<OrthographicPlanarCameraController> m_CameraController;

        Entity m_SquareEntity;
        Entity m_Camera0, m_Camera1;

        ShaderLibrary m_ShaderLibrary;
        Ref<Texture2D> m_TurretTexture;
        Ref<Texture2D> m_CheckerboardTexture;
        Ref<VertexArray> m_SquareVA;
        Ref<Texture2D> m_SpriteSheet;
        Ref<SubTexture2D> m_SpriteStairs, m_SpriteTree;

        Vector4 m_SquareColor = { 0.2f, 0.3f, 0.9f, 1.f };
        Vector4 m_TextureTint = { 0.2f, 0.2f, 0.2f, 1.f };
        Vector2 m_TextureScale = { 3.f, 3.f };
        float m_BackgroundRotation = 0.f;
    private:
        bool OnWindowResize(WindowResizeEvent& e);
    };

}
