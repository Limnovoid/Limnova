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
        bool OnKeyPressed(KeyPressedEvent& e);

        void NewScene();
        void OpenScene();
        void SaveSceneAs();
    private:
#ifdef LV_EDITOR_USE_ORBITAL
        Ref<OrbitalScene> m_Scene;
#else
        Ref<Scene> m_Scene;
#endif

        EditorCamera m_EditorCamera;

        Ref<Framebuffer> m_Framebuffer;
        Vector2 m_ViewportSize;
        bool m_ViewportFocused = false, m_ViewportHovered = false;

        SceneHierarchyPanel m_SceneHierarchyPanel;

        int m_ActiveGizmo = -1; /* from ImGuizmo::OPERATION */
        float m_SnapTranslate = 0.5f, m_SnapRotate = 45.f, m_SnapScale = 0.5f;
    };

}
