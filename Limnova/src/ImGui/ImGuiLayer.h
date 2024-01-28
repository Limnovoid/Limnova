#pragma once

#include "Core/Layer.h"

#include "Events/ApplicationEvent.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"


namespace Limnova
{

    class LIMNOVA_API ImGuiLayer : public Layer
    {
    public:
        /*** NOTE : enums must correspond to AddFontFromFileTTF() calls in ImGuiLayer::OnAttach() ***/
        enum FontIndex : uint32_t
        {
            Regular     = 0,
            Bold        = 1
        };
    public:
        ImGuiLayer();
        ~ImGuiLayer();

        void OnAttach() override;
        void OnDetach() override;
        void OnImGuiRender() override;
        void OnEvent(Event& e) override;

        void Begin();
        void End();

        void SetBlockEvents(bool block) { m_BlockEvents = block; }

        /// <summary>
        /// Set the cached path to the ImGui settings .ini file. Does not change preference for saving runtime settings (see ShouldSaveRuntimeSettings()).
        /// </summary>
        /// <param name="iniFilePath"></param>
        void SetIniFilePath(const std::filesystem::path& iniFilePath);

        /// <summary>
        /// Load ImGui settings (e.g. window size, layout) from .ini file.
        /// </summary>
        /// <param name="iniFilePath">Leave blank to load from cached file path (see SetIniFilePath()).</param>
        void LoadSettingsFromIniFile(const std::filesystem::path& iniFilePath = "");

        /// <summary>
        /// Whether ImGui should save changes to runtime settings (e.g. resizing and reordering windows).
        /// </summary>
        void ShouldSaveRuntimeSettings(bool value);

        void SetDarkTheme();
    private:
        bool m_BlockEvents = true;
        float m_Time = 0.f;
        char m_IniFilePathBuffer[256];
        bool m_shouldSaveRuntimeSettings;
    };

}
