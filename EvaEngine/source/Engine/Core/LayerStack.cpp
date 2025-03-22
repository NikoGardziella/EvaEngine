#include "pch.h"
#include "LayerStack.h"

namespace Engine {

	LayerStack::~LayerStack()
	{
		
		
		EE_CORE_INFO("~Layerstack. Index; {0}", m_LayerInsertIndex);
		m_Layers.clear();
	}

	void LayerStack::PushLayer(Layer* layer)
	{
		EE_PROFILE_FUNCTION();

		m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex, layer);
		m_LayerInsertIndex++;
		//layer->OnAttach();
	}

	void LayerStack::PushOverlay(Layer* overlay)
	{
		EE_PROFILE_FUNCTION();

		m_Layers.emplace_back(overlay);
		overlay->OnAttach();
	}

	void LayerStack::PopLayer(Layer* layer)
	{
		EE_PROFILE_FUNCTION();

		auto it = std::find(m_Layers.begin(), m_Layers.begin() + m_LayerInsertIndex, layer);
		if (it != m_Layers.begin() + m_LayerInsertIndex)
		{
			//layer->OnDetach(); // detached called in application
			m_Layers.erase(it);
			m_LayerInsertIndex--;
		}
	}

	void LayerStack::PopOverlay(Layer* overlay)
	{
		EE_PROFILE_FUNCTION();

		auto it = std::find(m_Layers.begin() + m_LayerInsertIndex, m_Layers.end(), overlay);
		if (it != m_Layers.end())
		{
			//overlay->OnDetach();
			m_Layers.erase(it);
		}
	}

}