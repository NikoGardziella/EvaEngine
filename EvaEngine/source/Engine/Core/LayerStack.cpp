#include "pch.h"
#include "LayerStack.h"

namespace Engine {

	
    LayerStack::~LayerStack()
    {
        std::cout << "Destroying LayerStack. Layers size: " << m_Layers.size() << std::endl;

        for (size_t i = 0; i < m_Layers.size(); ++i)
        {
            if (!m_Layers[i]) {
                std::cerr << "Warning: Null layer detected at index " << i << " during destruction!" << std::endl;
            }
            else {
                std::cout << "Destroying layer at index: " << i << " - " << typeid(*m_Layers[i]).name() << std::endl;
            }
        }

        m_Layers.clear();  // This ensures all unique_ptr destructors are called.
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

        if (!layer)  // Check if layer is null
        {
            EE_ERROR("Attempting to pop a null layer!");
            return;
        }

        // Ensure the index is valid
        if (m_LayerInsertIndex > 0 && m_LayerInsertIndex <= m_Layers.size())
        {
            auto it = std::find_if(m_Layers.begin(), m_Layers.begin() + m_LayerInsertIndex,
                [layer](const std::unique_ptr<Layer>& ptr) { return ptr.get() == layer; });

            if (it != m_Layers.begin() + m_LayerInsertIndex)
            {
                (*it)->OnDetach();
                m_Layers.erase(it);
                m_LayerInsertIndex--;
            }
        }
        else
        {
            EE_ERROR("Invalid Layer Insert Index!");
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