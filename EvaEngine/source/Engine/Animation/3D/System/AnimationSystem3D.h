#pragma once
#include <Engine/Animation/3D/SkeletonRegistry.h>
#include <Engine/Animation/3D/AnimationRegistry.h>
#include <Engine/Animation/3D/BonePaletteBuffer.h>


namespace Engine {

	class Scene;
	class AnimationSystem3D
	{
		void Update(Scene* scene, float dt, const SkeletonRegistry&, const AnimationRegistry&, BonePaletteBuffer&);

	};


}

