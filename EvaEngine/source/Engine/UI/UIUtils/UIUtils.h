#pragma once
#include <Engine/UI/UITransform2D.h>

namespace Engine {

	class UIUtils
	{
	public:
		static glm::vec2 UIUtils::Anchor01(UIAnchorPreset a);

		static glm::vec2 ComputeTopLeftPx(const UITransform2D& t, glm::vec2 screenPx);

	};
}

