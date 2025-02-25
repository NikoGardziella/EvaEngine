#pragma once

#include "Engine/Core/Layer.h"
#include "Engine/Events/Event.h"
#include "Engine/Events/MouseEvent.h"
#include "Engine/Events/KeyEvent.h"
#include "Engine/Events/ApplicationEvent.h"


namespace Engine {


	class EE_API ImGuiLayer : public Layer
	{

	public:
		ImGuiLayer();
		~ImGuiLayer();

		//virtual void OnUpdate() override;
		virtual void OnAttach() override;
		virtual void OnDetach() override;




		void OnEvent(Event& event);
		void Begin();
		void End();
		void SetDarkThemeColors();
		//void OnEvent(Event& event);

		void BlockEvents(bool block) { m_bockImGuiEvents = block; }
	private:
		float m_Time = 0.0f;
		bool m_bockImGuiEvents = true;
	};


}