#include "pch.h"
#include "SceneCamera.h"
#include "glm/gtc/matrix_transform.hpp"



namespace Engine {

	

	SceneCamera::SceneCamera()
	{
		RecalculateProjection();
	}

	SceneCamera::~SceneCamera()
	{
	}

	void SceneCamera::SetOrthographic(float size, float nearClip, float farClip)
	{
		m_projectionType = ProjectionType::Orthographic;
		m_orthographicSize = size;
		m_orthographicNear = nearClip;
		m_orthographicFar = farClip;

		RecalculateProjection();

	}

	void SceneCamera::SetPerspective(float verticalFOV, float nearClip, float farClip)
	{
		m_projectionType = ProjectionType::Perspective;
		m_perspectiveFOV = verticalFOV;
		m_perspectiveNear = nearClip;
		m_perspectiveFar = farClip;
		RecalculateProjection();
	}

	void SceneCamera::SetViewportSize(uint32_t width, uint32_t height)
	{
		if (height == 0) return; // Prevent division by zero
		m_aspectRatio = (float)width / (float)height;
		RecalculateProjection();
		m_viewportSize = { (float)width, (float)height };

	}

	void SceneCamera::SetViewportBounds(const std::array<glm::vec2, 2>& bounds)
	{
		m_viewportBounds = bounds;
		
	}
	glm::vec4 SceneCamera::CalculateCameraWorldBounds(const SceneCamera& Camera, const glm::mat4& cameraTransform)
	{
		glm::mat4 view = glm::inverse(cameraTransform);
		glm::mat4 proj = Camera.GetProjection();
		glm::mat4 invViewProj = glm::inverse(proj * view);

		// Vulkan NDC corners (Z = 1 for far plane)
		glm::vec4 ndcCorners[4] = {
			{-1, -1, 1, 1}, // bottom-left
			{ 1, -1, 1, 1}, // bottom-right
			{ 1,  1, 1, 1}, // top-right
			{-1,  1, 1, 1}  // top-left
		};

		glm::vec3 camPos = glm::vec3(cameraTransform[3]); // camera world position
		glm::vec2 minXY(FLT_MAX);
		glm::vec2 maxXY(-FLT_MAX);

		for (int i = 0; i < 4; ++i)
		{
			glm::vec4 cornerWorld = invViewProj * ndcCorners[i];
			cornerWorld /= cornerWorld.w;

			glm::vec3 farPoint = glm::vec3(cornerWorld);
			glm::vec3 rayDir = glm::normalize(farPoint - camPos);

			// Intersect ray with Z = 0 plane: camPos + t * rayDir
			float t = -camPos.z / rayDir.z;
			glm::vec3 hit = camPos + rayDir * t;

			minXY = glm::min(minXY, glm::vec2(hit));
			maxXY = glm::max(maxXY, glm::vec2(hit));
		}

		glm::vec2 size = maxXY - minXY;
		return glm::vec4(minXY, size); // x, y = minXY, z = width, w = height
	}



	void SceneCamera::RecalculateProjection()
	{
		if (m_projectionType == ProjectionType::Perspective)
		{
			m_projection = glm::perspective(m_perspectiveFOV, m_aspectRatio, m_perspectiveNear, m_perspectiveFar);
		}
		else
		{
			//Orthographic 
			float orthoLeft = -m_orthographicSize * m_aspectRatio * 0.5f;
			float orthoRight = m_orthographicSize * m_aspectRatio * 0.5f;

			float orthoBottom = -m_orthographicSize * 0.5f;
			float orthoTop = m_orthographicSize * 0.5f;


			m_projection = glm::ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, m_orthographicNear, m_orthographicFar);
		}
		

	}

}