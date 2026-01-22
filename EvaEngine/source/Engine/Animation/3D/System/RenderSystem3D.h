#pragma once
#include <Engine/Renderer/Camera.h>
#include <glm/glm.hpp>

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
		static void DebugDrawHitSphere2D_XY(const glm::mat4& enemyWorld, const glm::vec3& hitLocalCenter, float radius, const glm::vec4& color);
	};

}

