#pragma once

#include "Engine/Core/Core.h"
#include "Engine/Core/Layer.h"

#include <vector>

namespace Engine {

	class LayerStack
	{
	public:
		LayerStack() = default;
		~LayerStack();

		void PushLayer(std::unique_ptr<Layer> layer);
		void PushOverlay(std::unique_ptr<Layer> overlay);
		void PopLayer(Layer* layer);
		void PopOverlay(Layer* overlay);


		auto begin() { return m_Layers.begin(); }
		auto end() { return m_Layers.end(); }
		auto rbegin() { return m_Layers.rbegin(); }
		auto rend() { return m_Layers.rend(); }

		auto begin() const { return m_Layers.begin(); }
		auto end() const { return m_Layers.end(); }
		auto rbegin() const { return m_Layers.rbegin(); }
		auto rend() const { return m_Layers.rend(); }

		uint32_t GetLayerCount() { return m_Layers.size(); }
		const std::vector<std::unique_ptr<Layer>>& GetLayers() const { return m_Layers; }
	private:
		std::vector<Scope<Layer>> m_Layers;
		unsigned int m_LayerInsertIndex = 0;
	};

}