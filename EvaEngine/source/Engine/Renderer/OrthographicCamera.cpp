#include "pch.h"
#include "OrthographicCamera.h"

#include "glm/gtc/matrix_transform.hpp"

namespace Engine {

	OrthographicCamera::OrthographicCamera(float left, float right, float bottom, float top)
		: m_projectionMatrix(glm::ortho(left, right, bottom, top, -1.0f, 1.0f)),
		m_position(0.0f, 0.0f, 0.0f), // Default position
		m_rotation(0.0f)             // Default rotation
	{

		RecalculateViewMatrix();
		m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
	}

	void OrthographicCamera::SetProjection(float left, float right, float bottom, float top)
	{
		EE_PROFILE_FUNCTION();

		m_projectionMatrix = glm::ortho(left, right, bottom, top, -1.0f, 1.0f);
		m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;

	}

	void OrthographicCamera::RecalculateViewMatrix()
	{
		EE_PROFILE_FUNCTION();

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_position) *
			glm::rotate(glm::mat4(1.0f), glm::radians(m_rotation), glm::vec3(0.0f, 0.0f, 1.0f));

		// Invert the transform matrix for the view matrix
		m_viewMatrix = glm::inverse(transform);

		// Update the view-projection matrix
		m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;

	}
}