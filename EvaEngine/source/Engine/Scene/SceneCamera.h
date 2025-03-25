#pragma once
#include "Engine/Renderer/Camera.h"

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
	};

}
