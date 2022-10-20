#include "LayerStack.h"


namespace Limnova
{

	LayerStack::LayerStack()
	{
		m_LayerInsertOff = 0;
	}

	LayerStack::~LayerStack()
	{
		for (Layer* layer : m_Layers)
		{
			delete layer;
		}
	}


	void LayerStack::PushLayer(Layer* layer)
	{
		m_Layers.emplace(m_Layers.begin() + m_LayerInsertOff, layer);
		++m_LayerInsertOff;
	}


	void LayerStack::PushOverlay(Layer* overlay)
	{
		m_Layers.emplace_back(overlay);
	}


	void LayerStack::PopLayer(Layer* layer)
	{
		auto itOff = m_Layers.begin() + m_LayerInsertOff;
		auto it = std::find(m_Layers.begin(), itOff, layer);
		if (it != itOff)
		{
			m_Layers.erase(it);
			--m_LayerInsertOff;
		}
		else
		{
			LV_CORE_CRITICAL("Layer not found!");
		}
	}


	void LayerStack::PopOverlay(Layer* overlay)
	{
		auto it = std::find(m_Layers.begin() + m_LayerInsertOff, m_Layers.end(), overlay);
		if (it != m_Layers.end())
		{
			m_Layers.erase(it);
		}
		else
		{
			LV_CORE_CRITICAL("Overlay layer not found!");
		}
	}

}