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
            Edit = 0,
            Play = 1,
            Simulate = 2,
            Pause = 3
        };
    private:
        void UI_Toolbar();

        bool OnKeyPressed(KeyPressedEvent& e);
        bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

        bool CanMousePick();

        void NewScene();
        void OpenScene();
        void SaveScene();
        void SaveSceneAs();

        void OnScenePlay();
        void OnSceneSimulate();
        void OnSceneStop();

        void OnDuplicateEntity();
    private:
#ifdef LV_EDITOR_USE_ORBITAL
        Ref<OrbitalScene> m_ActiveScene;
        Ref<OrbitalScene> m_EditorScene;
#else
        Ref<Scene> m_ActiveScene;
        Ref<Scene> m_EditorScene;
#endif

        std::filesystem::path m_EditorScenePath;

        EditorCamera m_EditorCamera;

        Ref<Framebuffer> m_Framebuffer;
        Vector2 m_ViewportSize;
        Vector2 m_ViewportBounds[2];
        bool m_ViewportFocused = false, m_ViewportHovered = false;

        SceneHierarchyPanel m_SceneHierarchyPanel;

        SceneState m_SceneState = SceneState::Edit;
        Ref<Texture2D> m_IconPlay;
        Ref<Texture2D> m_IconPause;
        Ref<Texture2D> m_IconStop;

        float m_SceneDTMultiplier = 1.f;

        Entity m_HoveredEntity = Entity::Null;

        int m_ActiveGizmo = -1; /* from ImGuizmo::OPERATION */
        float m_SnapTranslate = 0.5f, m_SnapRotate = 45.f, m_SnapScale = 0.5f;

#if defined(LV_DEBUG) && defined(LV_EDITOR_USE_ORBITAL)
    private:
        static constexpr int kUpdateDurationPlotSpan = 360;
        std::array<float, kUpdateDurationPlotSpan> m_PhysicsUpdateDurations;
        int m_PhysicsUpdateDurationsOffset = 0;

        static constexpr int kObjPlotSpan = 12;
        typedef std::vector<std::array<float, kObjPlotSpan>> TObjDataMatrix;
        TObjDataMatrix m_ObjectUpdates;
        int m_ObjectUpdatesOffset = 0;
        TObjDataMatrix m_DurationErrors;
        std::vector<size_t> m_DurationErrorsOffsets = {};

        std::function<void(TObjDataMatrix&, size_t, float)> m_ResizeInit = [this](TObjDataMatrix& data, size_t size, float val) {
            size_t i = data.size();
            data.resize(size);
            for (; i < size; i++) {
                for (size_t j = 0; j < kObjPlotSpan; j++) data[i][j] = val;
            }
        };
#endif
    };

}
