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

		void OnUpdate();
		void OnEvent(Event& event);
		void OnAttach();
		void OnDetach();

	private:
		bool OnMouseButtonPressedEvent(MouseButtonPressedEvent& e);
		bool OnMouseButtonReleasedEvent(MouseButtonReleasedEvent& e);
		bool OnMouseMovedEvent(MouseMovedEvent& e);
		bool OnMouseScrollEvent(MouseScrolledEvent& e);
		
		bool OnKeyReleasedEvent(KeyReleasedEvent& e);
		bool OnKeyPressedEvent(KeyPressedEvent& e);
		bool OnKeyTypedEvent(KeyTypedEvent& e);
		
		bool OnWindowResizedEvent(WindowResizeEvent& e);


	private:
		float m_Time = 0.0f;

	};


}