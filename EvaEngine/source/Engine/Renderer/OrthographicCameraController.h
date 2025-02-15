#pragma once
#include <Engine/Renderer/OrthographicCamera.h>
#include <Engine/Core/Timestep.h>
#include "Engine/Events/ApplicationEvent.h"
#include "Engine/Events/MouseEvent.h"


namespace Engine
{
	class OrthographicCameraController
	{
	public:
		OrthographicCameraController(float aspectratio, bool rotation = false);

		void OnUpdate(Timestep timestep);
		void OnEvent(Event& event);

		OrthographicCamera& GetCamera() { return m_camera;  }
		const OrthographicCamera& GetCamera() const { return m_camera;  }

		void SetZoomLevel(float level) { m_zoomLevel = level;  }
		float GetZoomLevel() { return m_zoomLevel;  }

	private:

		bool OnMouseScrolled(MouseScrolledEvent& event);
		bool OnWindowResize(WindowResizeEvent& event);

	private:
		float m_aspectRatio;
		float m_zoomLevel = 1.0f;
		OrthographicCamera m_camera;

		bool m_rotation;
		float m_cameraTranslationSpeed = 1.0f;
		float m_cameraRotationSpeed = 100.0f;
		float m_cameraRotation = 0.0f;
		glm::vec3 m_cameraPosition = { 0.0f, 0.0f, 0.0f };

	};
}

