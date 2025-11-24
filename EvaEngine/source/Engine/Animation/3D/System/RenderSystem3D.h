#pragma once
#include <Engine/Renderer/Camera.h>

namespace Engine {

	class Scene;
	class MaterialRegistry;
	class MeshRegistry;
	class VisibleSet;
	class SceneCamera;
	class TransformSystem3D;
	class RenderSystem3D
	{
	public:
		void Render(const VisibleSet& vis, Scene* scene, const TransformSystem3D& xforms, const MeshRegistry& meshes, const MaterialRegistry& materials);
	};

}

