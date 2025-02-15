#pragma once
#include "Engine/Core/Core.h"


namespace Engine{


	class EE_API Input
	{

	public:
		inline static bool IsKeyPressed(int keycode) { return s_instance->IsKeyPressedImplentation(keycode); }

		inline static bool IsMouseButtonyPressed(int button) { return s_instance->IsMouseButtonPressedImplentation(button); }
		inline static std::pair<float, float> GetMousePosition() { return s_instance->GetMousePositionImplentation(); }
		inline static float GetMouseX() { return s_instance->GetMouseXImplentation(); }
		inline static float GetMouseY() { return s_instance->GetMouseYImplentation(); }

	protected:
		virtual bool IsKeyPressedImplentation(int keycode) = 0;
		virtual bool IsMouseButtonPressedImplentation(int keycode) = 0;
		virtual std::pair<float, float> GetMousePositionImplentation() = 0;
		virtual float GetMouseXImplentation() = 0;
		virtual float GetMouseYImplentation() = 0;

	private:
		static Input* s_instance;


	};
}