#pragma once

#include "Layer.h"


namespace Limnova
{

    class LIMNOVA_API LayerStack
    {
    public:
        LayerStack();
        ~LayerStack();

        void PushLayer(Layer* layer);
        void PushOverlay(Layer* overlay);
        void PopLayer(Layer* layer);
        void PopOverlay(Layer* overlay);

        using LStack = std::vector<Layer*>;
        LStack::iterator begin() { return m_Layers.begin(); }
        LStack::iterator end() { return m_Layers.end(); }
        LStack::reverse_iterator rbegin() { return m_Layers.rbegin(); }
        LStack::reverse_iterator rend() { return m_Layers.rend(); }
    private:
        LStack m_Layers;
        int m_LayerInsertOff;
    };

}