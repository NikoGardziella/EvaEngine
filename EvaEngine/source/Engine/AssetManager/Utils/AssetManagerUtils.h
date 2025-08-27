#pragma once

namespace Engine {

	class AssetManagerUtils
	{
	public:
		static void AssetManagerUtils::ComputePivotFromAlpha(const uint8_t* rgba, int w, int h, int alphaThresh,
			int& outPivotYOffsetPx, int& outPivotXCenterOffsetPx);
	};

}

