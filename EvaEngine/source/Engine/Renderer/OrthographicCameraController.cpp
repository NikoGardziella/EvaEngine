#include "pch.h"
#include "OrthographicCameraController.h"
#include <Engine/Core/KeyCodes.h>
#include "Engine/Core/Input.h"
#include "Engine/Core/Core.h"
//#include "Engine/Events/MouseEvent.h"
//#include "Engine/Events/ApplicationEvent.h"


namespace Engine {

	OrthographicCameraController::OrthographicCameraController(float aspectratio, bool rotation)
		: m_aspectRatio(aspectratio),
		m_camera(-m_aspectRatio * m_zoomLevel, m_aspectRatio * m_zoomLevel, -m_zoomLevel, m_zoomLevel),
		m_rotation(rotation)
	{

	}

	void OrthographicCameraController::OnUpdate(Timestep timestep)
	{
		EE_PROFILE_FUNCTION();

		// *********** CAMERA MOVEMENT ***********
		if (Input::IsKeyPressed(EE_KEY_A))
		{
			m_cameraPosition.x -= m_cameraTranslationSpeed * timestep.GetSeconds();

		}
		else if (Input::IsKeyPressed(EE_KEY_D))
		{
			m_cameraPosition.x += m_cameraTranslationSpeed * timestep.GetSeconds();

		}
		if (Input::IsKeyPressed(EE_KEY_W))
		{
			m_cameraPosition.y += m_cameraTranslationSpeed * timestep.GetSeconds();

		}
		else if (Input::IsKeyPressed(EE_KEY_S))
		{
			m_cameraPosition.y -= m_cameraTranslationSpeed * timestep.GetSeconds();
		}
		if (m_rotation)
		{
			if (Input::IsKeyPressed(EE_KEY_Q))
			{
				m_cameraRotation -= m_cameraRotationSpeed * timestep.GetSeconds();
			}
			else if (Input::IsKeyPressed(EE_KEY_E))
			{
				m_cameraRotation += m_cameraRotationSpeed * timestep.GetSeconds();
			}
			m_camera.SetRotation(m_cameraRotation);
		}
		m_cameraTranslationSpeed = m_zoomLevel;
		m_camera.SetPosition(m_cameraPosition);
	}

	void OrthographicCameraController::OnEvent(Event& event)
	{
		EE_PROFILE_FUNCTION();

		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<MouseScrolledEvent>(EE_BIND_EVENT_FN(OrthographicCameraController::OnMouseScrolled));
		dispatcher.Dispatch<WindowResizeEvent>(EE_BIND_EVENT_FN(OrthographicCameraController::OnWindowResize));
	}

	bool OrthographicCameraController::OnMouseScrolled(MouseScrolledEvent& event)
	{
		EE_PROFILE_FUNCTION();

		m_zoomLevel -= event.GetYOffset() * 0.25f;

		m_zoomLevel = std::max(m_zoomLevel, 0.25f);

		m_camera.SetProjection(-m_aspectRatio * m_zoomLevel, m_aspectRatio * m_zoomLevel, -m_zoomLevel, m_zoomLevel);

		return false;
	}
	bool OrthographicCameraController::OnWindowResize(WindowResizeEvent& event)
	{
		EE_PROFILE_FUNCTION();

		if (event.GetHeight() == 0) return false; // Prevent division by zero

		m_aspectRatio = static_cast<float>(event.GetWidth()) / static_cast<float>(event.GetHeight());
		
		m_camera.SetProjection(-m_aspectRatio * m_zoomLevel,
			m_aspectRatio * m_zoomLevel,
			-m_zoomLevel,
			m_zoomLevel);

		return false;
	}
}