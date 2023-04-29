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
        enum class SceneState
        {
            Edit = 0, Play = 1
        };
    private:
        void UI_Toolbar();

        bool OnKeyPressed(KeyPressedEvent& e);
        bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

        bool CanMousePick();

        void NewScene();
        void OpenScene();
        void SaveSceneAs();

        void OnScenePlay();
        void OnSceneStop();
    private:
#ifdef LV_EDITOR_USE_ORBITAL
        Ref<OrbitalScene> m_Scene;
#else
        Ref<Scene> m_Scene;
#endif

        EditorCamera m_EditorCamera;

        Ref<Framebuffer> m_Framebuffer;
        Vector2 m_ViewportSize;
        Vector2 m_ViewportBounds[2];
        bool m_ViewportFocused = false, m_ViewportHovered = false;

        SceneHierarchyPanel m_SceneHierarchyPanel;

        SceneState m_SceneState = SceneState::Edit;
        Ref<Texture2D> m_IconPlay;
        Ref<Texture2D> m_IconStop;

        Entity m_HoveredEntity = Entity::Null;

        int m_ActiveGizmo = -1; /* from ImGuizmo::OPERATION */
        float m_SnapTranslate = 0.5f, m_SnapRotate = 45.f, m_SnapScale = 0.5f;
    };

}
