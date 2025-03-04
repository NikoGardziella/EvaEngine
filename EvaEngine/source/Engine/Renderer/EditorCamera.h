#pragma once

#include "Engine/Renderer/Camera.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <Engine/Core/Timestep.h>
#include <Engine/Events/Event.h>
#include <Engine/Events/MouseEvent.h>

namespace Engine {



    class EditorCamera : public Camera
    {
    public:
        EditorCamera() = default;
        EditorCamera(float fov, float aspectRatio, float nearClip, float farClip);

        void UpdateProjection();

        void UpdateView();

        glm::quat GetOrientation() const;

        void OnUpdate(Timestep timestep);
        void OnMouseZoom(float deltaY);
        void OnEvent(Event& event);

        bool OnMouseScroll(MouseScrolledEvent& event);

        float GetDistance() const { return m_distance;  }
        void SetDistance(float distance) { m_distance = distance;  }

        void SetViewportSize(float width, float height);

        // Getters
        glm::mat4 GetViewMatrix() const { return m_viewMatrix; }
        glm::mat4 GetProjectionMatrix() const { return m_projection; }
        glm::mat4 GetViewProjection() const { return m_projection * m_viewMatrix; }
        glm::vec3 GetPosition() const { return m_position; }
        glm::vec3 GetRotation() const { return glm::vec3(m_pitch, m_yaw, 0.0f); }
        glm::vec3 GetForwardDirection() const;
        glm::vec3 GetRightDirection() const;
        glm::vec3 GetUpDirection() const;
        glm::vec2 PanSpeed() const;
        float GetFOV() const { return m_FOV; }
        float GetAspectRatio() const { return m_aspectRatio; }
        float GetNearClip() const { return m_nearClip; }
        float GetFarClip() const { return m_farClip; }

        // Setters
        void SetPosition(const glm::vec3& position);
        void SetRotation(float pitch, float yaw);
        void SetProjection(float fov, float aspectRatio, float nearClip, float farClip);

    private:
        void UpdateViewMatrix();
        void UpdateProjectionMatrix();

        //void OnMouseScroll(const ImVec2& delta);
        void OnMousePan(const glm::vec2& delta);
        void OnMouseRotate(const glm::vec2& delta);
    private:
        glm::mat4 m_viewMatrix;

        glm::vec3 m_position = { 0.0f, 0.0f, 3.0f };
        glm::vec2 m_initialMousePosition;
        glm::vec3 m_focalPoint = { 0.0f, 0.0f, 0.0f };

        float m_pitch = 0.0f;  // Up/down rotation
        float m_yaw = -90.0f;  // Left/right rotation

        float m_FOV = 45.0f;
        float m_aspectRatio = 1.78f;
        float m_nearClip = 0.1f;
        float m_farClip = 1000.0f;

        float m_distance = 10.0f;
        float m_viewportWidth = 1280;
        float m_viewportHeight = 720;
    };


}

