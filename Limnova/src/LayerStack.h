#pragma once

#include "Core.h"
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
	private:
		LStack m_Layers;
		LStack::iterator m_LayerInsert;
		int m_LayerInsertOff;
	};

}