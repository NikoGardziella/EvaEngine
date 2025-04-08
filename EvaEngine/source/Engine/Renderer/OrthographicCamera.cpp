#include "pch.h"
#include "OrthographicCamera.h"

#include "glm/gtc/matrix_transform.hpp"

namespace Engine {

	OrthographicCamera::OrthographicCamera(float left, float right, float bottom, float top)
		: m_position(0.0f, 0.0f, 0.0f),
		m_rotation(0.0f)
	{
		SetProjection(left, right, bottom, top);
	}

	void OrthographicCamera::SetProjection(float left, float right, float bottom, float top)
	{
		EE_PROFILE_FUNCTION();

		m_projectionMatrix = glm::ortho(left, right, bottom, top, -1.0f, 1.0f);
		RecalculateViewMatrix();
	}

	void OrthographicCamera::RecalculateViewMatrix()
	{
		EE_PROFILE_FUNCTION();

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_position) *
			glm::rotate(glm::mat4(1.0f), glm::radians(m_rotation), glm::vec3(0.0f, 0.0f, 1.0f));

		m_viewMatrix = glm::inverse(transform);
		m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
	}

}
