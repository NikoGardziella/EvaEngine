#pragma once
#include <Engine/Renderer/Camera.h>
#include <Engine/Core/Core.h>
#include <Engine/Map/FogOfWar/FogOfWar.h>

namespace Engine {


	class Scene;
	class TransformSystem3D;
	class VisibleSet;
	class SceneCamera;
	class CullingSystem3D
	{
		public:
			VisibleSet BuildVisible(Scene* scene, const Camera& cam, const TransformSystem3D& xforms, const glm::mat4& cameraWorld, Ref<FogOfWar> fogOfwarRef);
	};


}

