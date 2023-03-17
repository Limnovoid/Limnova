#pragma once

#include <Limnova.h>

#include "Panels/SceneHierarchyPanel.h"


namespace Limnova
{

    class EditorLayer : public Layer
    {
    public:
        EditorLayer();
        virtual ~EditorLayer() = default;

        void OnAttach() override;
        void OnDetach() override;

        void OnUpdate(Timestep dT) override;
        void OnImGuiRender() override;
        void OnEvent(Event& e) override;
    private:
        Ref<Scene> m_Scene;

        Ref<PerspectivePlanarCameraController> m_CameraController;
        Ref<Framebuffer> m_Framebuffer;
        Vector2 m_ViewportSize;
        bool m_ViewportFocused = false, m_ViewportHovered = false;

        Entity m_SquareEntity;
        Entity m_Camera0, m_Camera1;

        SceneHierarchyPanel m_SceneHierarchyPanel;

        //Ref<Limnova::Texture2D> m_CheckerboardTexture;
        //Ref<Limnova::Texture2D> m_SpriteSheet;
        //Ref<Limnova::SubTexture2D> m_SpriteStairs, m_SpriteTree;
        //
        //Vector4 m_SquareColor = { 0.2f, 0.3f, 0.9f, 1.f };
        //Vector4 m_TextureTint = { 0.2f, 0.2f, 0.2f, 1.f };
        //Vector2 m_TextureScale = { 3.f, 3.f };
        //float m_BackgroundRotation = 0.f;
    };

}
