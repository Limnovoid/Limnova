#pragma once

#include <Limnova.h>

namespace Limnova
{

    class SceneHierarchyPanel
    {
    public:
        SceneHierarchyPanel() = default;
        SceneHierarchyPanel(Scene* scene);

        void SetContext(Scene* scene);

        void SetSelectedEntity(Entity entity);
        Entity GetSelectedEntity() const { return m_SelectedEntity; }

        void OnImGuiRender();
    private:
        void EntityNode(Entity entity, bool forceExpanded = false);
        void Inspector(Entity entity);
    private:
        Scene* m_Scene;
        Entity m_SelectedEntity;
    };

    namespace LimnGui
    {
        /// <summary>
        /// If previous item is hovered, displays a tooltip with the text contained in 'description'.
        /// </summary>
        /// <param name="info">The information string to display in the tooltip</param>
        void ItemDescription(const std::string &description);

        /// <summary>
        /// On the same line as the previous item, displays a greyed-out (disabled) "(?)" which, when hovered, displays a tooltip with the text contained in 'description'.
        /// </summary>
        /// <param name="description">The text to display in the tooltip</param>
        void HelpMarker(const std::string& description);

        template<typename T>
        struct InputConfig
        {
            T ResetValue = 0;
            T Speed = 1;
            T FastSpeed = 10;
            T Min = 0;
            T Max = 0;
            uint32_t Precision = 3;
            bool Scientific = false;
            bool ReadOnly = false;
            size_t WidgetId = 0;
            float LabelWidth = 100.f;
            float WidgetWidth = 100.f;
            std::string HelpMarker = {};

            InputConfig() = default;
            InputConfig(
                T resetValue,
                T speed = 1,
                T fastSpeed = 10,
                T min = 0,
                T max = 0,
                uint32_t precision = 3,
                bool scientific = false,
                bool readOnly = false,
                size_t widgetId = 0,
                float labelWidth = 100.f,
                float widgetWidth = 100.f,
                std::string helpMarker = {}
            ) :
                ResetValue(resetValue),
                Speed(speed),
                FastSpeed(fastSpeed),
                Min(min),
                Max(max),
                Precision(precision),
                Scientific(scientific),
                ReadOnly(readOnly),
                WidgetId(widgetId),
                LabelWidth(labelWidth),
                WidgetWidth(widgetWidth),
                HelpMarker(helpMarker) {}
        };

        bool Checkbox(const std::string& label, bool& value, float columnWidth = 100.f);
        bool InputInt(const std::string& label, int& value, const InputConfig<int>& config = {});
        bool InputUInt64(const std::string& label, uint64_t& value, const InputConfig<uint64_t>& config = {});
        bool InputScientific(const std::string& label, double& value, float columnWidth = 100.f);
        bool InputDouble(const std::string& label, double& value, const InputConfig<double>& config, float columnWidth = 100.f);
        bool DragInt(const std::string& label, int& value, const InputConfig<int>& config, float columnWidth = 100.f);
        bool DragFloat(const std::string& label, float& value, const InputConfig<float>& config, float columnWidth = 100.f);
        bool DragVec3(const std::string& label, Vector3& values, const InputConfig<float>& config, float columnWidth = 100.f);
        bool InputVec3d(const std::string& label, Vector3d& values, const InputConfig<double>& config, float columnWidth = 100.f);
        bool ColorEdit(const std::string& label, Vector4& values, float columnWidth = 100.f);
        bool ColorEdit3(const std::string& label, Vector3& values, float columnWidth = 100.f);
        bool SliderFloat(const std::string& label, float& value, const InputConfig<float>& config = {}, bool logarithmic = false);
        bool TextEdit(const std::string &label, char *textBuffer, size_t bufferSize, float columnWidth = 100.f);
    }

}
