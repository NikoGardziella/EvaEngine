#pragma once

#include "Engine/Core/Input.h"

namespace Engine {


	class WindowsInput : public Input
	{


	protected:
		virtual bool IsKeyPressedImplentation(int keycode) override;
		virtual bool IsMouseButtonPressedImplentation(int keycode) override;
		virtual std::pair<float, float> GetMousePositionImplentation() override;
		virtual float GetMouseXImplentation() override;
		virtual float GetMouseYImplentation() override;


	};

}