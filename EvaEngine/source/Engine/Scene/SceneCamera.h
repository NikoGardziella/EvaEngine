#pragma once
#include "Engine/Renderer/Camera.h"
#include "Engine/Core/Input.h"
#include "Engine/Debug/Instrumentor.h"

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


		glm::vec2 GetViewportSize() const { return m_viewportSize; }

		void SetOrthographic(float size, float nearClip, float farClip);
		void SetPerspective(float verticalFOV, float nearClip, float farClip);

		void SetViewportSize(uint32_t width, uint32_t height);
		void SetViewportBounds(const std::array<glm::vec2, 2>& bounds);

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

		bool IsAABBVisible(const glm::vec2& min, const glm::vec2& max) const
		{
			EE_PROFILE_FUNCTION();

			const glm::vec2& viewMin = m_viewportBounds[0];
			const glm::vec2& viewMax = m_viewportBounds[1];

			return !(max.x < viewMin.x || min.x > viewMax.x ||
				max.y < viewMin.y || min.y > viewMax.y);
		}

		glm::vec2 ScreenToWorld(glm::mat4 cameraTransform)
		{
			glm::vec2 mouseScreen;
			// there is better way to do this. consider setting bounds to client
			if (m_viewportBounds[0].x > 0.0f)
			{

				// for editor
				mouseScreen = Engine::Input::GetMouseScreenPosition();
			}
			else
			{
				// for client onyl
				mouseScreen = Engine::Input::GetMousePosition();

			}
			 

			if (m_projectionType == ProjectionType::Orthographic)
			{
				// this is not tested

				glm::mat4 view = glm::inverse(cameraTransform);

				float x = (2.0f * mouseScreen.x) / m_viewportSize.x - 1.0f;
				float y = 1.0f - (2.0f * mouseScreen.y) / m_viewportSize.y; // Invert Y for Vulkan (origin top-left)
				glm::vec4 ndc = glm::vec4(x, y, 0.0f, 1.0f);

				glm::mat4 invVP = glm::inverse(m_projection * view);

				glm::vec4 world = invVP * ndc;

				if (world.w != 0.0f)
					world /= world.w;


				return glm::vec2(world.x, world.y);
			}
			else if(m_projectionType == ProjectionType::Perspective)
			{
				glm::vec2 mouseInViewport = mouseScreen - m_viewportBounds[0];

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

		glm::vec2 SceneCamera::WorldToScreen(const glm::vec3& worldPosition, const glm::mat4& cameraTransform) const
		{
			EE_PROFILE_FUNCTION();


			glm::mat4 viewMatrix = glm::inverse(cameraTransform);
			glm::mat4 viewProj = m_projection * viewMatrix;

			glm::vec4 clipSpacePos = viewProj * glm::vec4(worldPosition, 1.0f);

			// Perspective divide
			glm::vec3 ndc = glm::vec3(clipSpacePos) / clipSpacePos.w;

			// Convert NDC (-1 to 1) to screen space (0 to viewportSize)
			glm::vec2 screenSpace;
			screenSpace.x = ((ndc.x + 1.0f) / 2.0f) * m_viewportSize.x;
			screenSpace.y = ((1.0f - ndc.y) / 2.0f) * m_viewportSize.y; // Flip Y axis

			// Offset to match full screen space (ImGui global position)
			screenSpace += m_viewportBounds[0];

			return screenSpace;
		}
		//struct CameraComponent;
		//struct TransformComponent;


		glm::vec4 CalculateCameraWorldBounds(const SceneCamera& Camera, const glm::mat4& cameraTransform);
		

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
		std::array<glm::vec2, 2> m_viewportBounds = { glm::vec2(0, 0), glm::vec2(1, 1) };

		
	};

}
