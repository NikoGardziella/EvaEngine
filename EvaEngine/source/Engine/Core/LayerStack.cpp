#include "pch.h"
#include "LayerStack.h"
#include "Application.h"



namespace Engine {

	
    LayerStack::~LayerStack()
    {
        EE_TRACE("Destroying LayerStack: {}", (void*)this);  // Debug log
        
        
    }



	
	void LayerStack::PushLayer(std::unique_ptr<Layer> layer)
	{
		EE_PROFILE_FUNCTION();

		m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex, std::move(layer));
		m_LayerInsertIndex++;
	}

	void LayerStack::PushOverlay(std::unique_ptr<Layer> overlay)
	{
		EE_PROFILE_FUNCTION();

		overlay->OnAttach();
		m_Layers.emplace_back(std::move(overlay));
	}

    void LayerStack::PopLayer(Layer* layer)
    {
        EE_PROFILE_FUNCTION();
        
        auto it = std::find_if(m_Layers.begin(), m_Layers.begin() + m_LayerInsertIndex,
            [layer](const std::unique_ptr<Layer>& ptr) { return ptr.get() == layer; });

        if (it != m_Layers.begin() + m_LayerInsertIndex)
        {
            (*it)->OnDetach();   // Ensure proper cleanup
            m_Layers.erase(it);  // Smart pointer automatically frees memory
            if (m_LayerInsertIndex > 0)
                m_LayerInsertIndex--; // Prevent underflow
        }
        
    }

    void LayerStack::PopOverlay(Layer* overlay)
    {
        EE_PROFILE_FUNCTION();
        if (!overlay)  // Check if overlay is null
        {
            EE_ERROR("Attempting to pop a null overlay!");
            return;
        }

        // Ensure the index is valid
        if (m_LayerInsertIndex >= 0 && m_LayerInsertIndex < m_Layers.size())
        {
            auto it = std::find_if(m_Layers.begin() + m_LayerInsertIndex, m_Layers.end(),
                [overlay](const std::unique_ptr<Layer>& existingLayer)
                {
                    return existingLayer.get() == overlay;
                });

            if (it != m_Layers.end())
            {
                (*it)->OnDetach();
                m_Layers.erase(it);
            }
        }
        else
        {
            EE_ERROR("Invalid Overlay Insert Index!");
        }
    }



}