#pragma once
#include "Engine/Renderer/Camera.h"
#include "glm/glm.hpp"

namespace Engine {

	class SceneCamera : public Camera
	{

	public:

		enum class ProjectionType
		{
			Perspective = 0,
			Orthographic = 1,
		};

	public:
		SceneCamera();
		virtual ~SceneCamera();


		void SetOrthographic(float size, float nearClip, float farClip);
		void SetPerspective(float verticalFOV, float nearClip, float farClip);

		void SetViewportSize(uint32_t width, uint32_t height);
		void SetViewportBounds(glm::vec2 minBounds);

		ProjectionType GetProjectionType() const { return m_projectionType; }
		void SetProjectionType(ProjectionType type) { m_projectionType = type; }


		// ********* Orthographic Camera ************
		float GetOrthographicSize() const { return m_orthographicSize; }
		void SetOrthographicSize(float size)
		{
			if (m_orthographicSize != size)
			{
				m_orthographicSize = size;
				RecalculateProjection();
			}
		}

		float GetOrthographicNearClip() const { return m_orthographicNear; }
		void SetOrthographicNearClip(float nearClip)
		{
			if (m_orthographicNear != nearClip)
			{
				m_orthographicNear = nearClip;
				RecalculateProjection();
			}
		}

		float GetOrthographicFarClip() const { return m_orthographicFar; }
		void SetOrthographicFarClip(float farClip)
		{
			if (m_orthographicFar != farClip)
			{
				m_orthographicFar = farClip;
				RecalculateProjection();
			}
		}


		// ********* Perspective Camera ************
		float GetPerspectiveFOV() const { return m_perspectiveFOV; }
		void SetPerspectiveFOV(float fov)
		{
			
			m_perspectiveFOV = fov;
			
			RecalculateProjection();
		}

		float GetPerspectiveNearClip() const { return m_perspectiveNear; }
		void SetPerspectiveNearClip(float nearClip)
		{
			if (m_perspectiveNear != nearClip)
			{
				m_perspectiveNear = nearClip;
				RecalculateProjection();
			}
		}

		float GetPerspectiveFarClip() const { return m_perspectiveFar; }
		void SetPerspectiveFarClip(float farClip)
		{
			if (m_perspectiveFar != farClip)
			{
				m_perspectiveFar = farClip;
				RecalculateProjection();
			}
		}

		glm::vec2 ScreenToWorld(const glm::vec2& screenPos, glm::mat4 cameraTransform)
		{
			

			if (m_projectionType == ProjectionType::Orthographic)
			{

				glm::mat4 view = glm::inverse(cameraTransform);

				float x = (2.0f * screenPos.x) / m_viewportSize.x - 1.0f;
				float y = 1.0f - (2.0f * screenPos.y) / m_viewportSize.y; // Invert Y for Vulkan (origin top-left)
				glm::vec4 ndc = glm::vec4(x, y, 0.0f, 1.0f);

				glm::mat4 invVP = glm::inverse(m_projection * view);

				glm::vec4 world = invVP * ndc;

				if (world.w != 0.0f)
					world /= world.w;


				return glm::vec2(world.x, world.y);
			}
			else if(m_projectionType == ProjectionType::Perspective)
			{
				glm::vec2 mouseInViewport = screenPos - m_viewportBounds[0];

				float x = (2.0f * mouseInViewport.x) / m_viewportSize.x - 1.0f;
				float y = 1.0f - (2.0f * mouseInViewport.y) / m_viewportSize.y;
				glm::vec4 ndcFar = glm::vec4(x, y, 1.0f, 1.0f);
				glm::vec4 ndcNear = glm::vec4(x, y, -1.0f, 1.0f);

				glm::mat4 view = glm::inverse(cameraTransform);
				glm::mat4 invVP = glm::inverse(m_projection * view);

				glm::vec4 worldFar = invVP * ndcFar;  worldFar /= worldFar.w;
				glm::vec4 worldNear = invVP * ndcNear; worldNear /= worldNear.w;

				glm::vec3 rayOrigin = glm::vec3(worldNear);
				glm::vec3 rayDir = glm::normalize(glm::vec3(worldFar - worldNear));

				if (fabs(rayDir.z) < 1e-6f)
					return glm::vec2(rayOrigin.x, rayOrigin.y);

				float t = -rayOrigin.z / rayDir.z;
				glm::vec3 intersection = rayOrigin + t * rayDir;
				return glm::vec2(intersection.x, intersection.y);

			}
			
		}



	private:
		void RecalculateProjection();

	private:

		ProjectionType m_projectionType = ProjectionType::Orthographic;

		// Orthographics camera propertios
		float m_orthographicSize = 10.0f;
		float m_orthographicNear = -1.0f;
		float m_orthographicFar = 1.0f;

		// Perspective Camera Properties
		float m_perspectiveFOV = 45.0f;
		float m_perspectiveNear = 0.1f;
		float m_perspectiveFar = 1000.0f;

		float m_aspectRatio = 2.0f;
		glm::vec2 m_viewportSize = { 0.0f, 0.0f };
		glm::vec2 m_viewportBounds[2] = { { 0.0f, 0.0f }, { 1.0f, 1.0f } };
	};

}
