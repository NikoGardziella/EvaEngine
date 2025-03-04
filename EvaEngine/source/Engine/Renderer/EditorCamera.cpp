#include "pch.h"
#include "EditorCamera.h"

#include "Engine/Core/Input.h"
#include <Engine/Events/MouseEvent.h>

#include "Engine/Core/Core.h"

namespace Engine {

    EditorCamera::EditorCamera(float fov, float aspectRatio, float nearClip, float farClip)
        : m_FOV(fov), m_aspectRatio(aspectRatio), m_nearClip(nearClip), m_farClip(farClip)
    {
        UpdateProjection();
        UpdateView();
    }

    void EditorCamera::UpdateProjection()
    {
        m_projection = glm::perspective(glm::radians(m_FOV), m_aspectRatio, m_nearClip, m_farClip);
    }

    void EditorCamera::UpdateView()
    {
        glm::vec3 front;
        front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
        front.y = sin(glm::radians(m_pitch));
        front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
        front = glm::normalize(front);

        glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
        glm::vec3 up = glm::normalize(glm::cross(right, front));

        m_position = m_focalPoint - GetForwardDirection() * m_distance;
        m_viewMatrix = glm::lookAt(m_position, m_position + front, up);
    }

    void EditorCamera::OnUpdate(Timestep timestep)
    {
        if (Input::IsKeyPressed(Key::LeftAlt))
        {
            const glm::vec2 mouse{ Input::GetMouseX(), Input::GetMouseY() };
            glm::vec2 delta = (mouse - m_initialMousePosition) * 0.003f;
            m_initialMousePosition = mouse;

            if (Input::IsMouseButtonPressed(Mouse::ButtonMiddle))
            {
                OnMousePan(delta);
            }
            else if (Input::IsMouseButtonPressed(Mouse::ButtonLeft))
            {
                OnMouseRotate(delta);
            }
            else if (Input::IsMouseButtonPressed(Mouse::ButtonRight))
            {
                OnMouseZoom(delta.y);
            }
        }
        UpdateView();
    }

    void EditorCamera::OnEvent(Event& event)
    {
        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<MouseScrolledEvent>(EE_BIND_EVENT_FN(EditorCamera::OnMouseScroll));
    }

    bool EditorCamera::OnMouseScroll(MouseScrolledEvent& event)
    {
        float delta = event.GetYOffset() * 1.1f;
        OnMouseZoom(delta);
        UpdateView();
        return true;
    }

    
    
    void EditorCamera::OnMouseZoom(float deltaY)
    {
        m_FOV -= deltaY;
        if (m_FOV < 10.0f) m_FOV = 10.0f;
        if (m_FOV > 90.0f) m_FOV = 90.0f;

        UpdateProjection();
    }

    void EditorCamera::OnMousePan(const glm::vec2& delta)
    {
        glm::vec2 panSpeed = PanSpeed();
        m_focalPoint += -GetRightDirection() * delta.x * panSpeed.x;
        m_focalPoint += GetUpDirection() * delta.y * panSpeed.y;


        UpdateView();
    }

    void EditorCamera::OnMouseRotate(const glm::vec2& delta)
    {
        float sensitivity = 20.1f;
        m_yaw += delta.x * sensitivity;
        m_pitch -= delta.y * sensitivity;

        if (m_pitch > 89.0f) m_pitch = 89.0f;
        if (m_pitch < -89.0f) m_pitch = -89.0f;

        UpdateView();
    }

    glm::vec3 EditorCamera::GetForwardDirection() const
    {
        return glm::normalize(glm::vec3(
            cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch)),
            sin(glm::radians(m_pitch)),
            sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch))
        ));
    }

    glm::vec3 EditorCamera::GetRightDirection() const
    {
        return glm::normalize(glm::cross(GetForwardDirection(), glm::vec3(0.0f, 1.0f, 0.0f)));
    }

    glm::vec3 EditorCamera::GetUpDirection() const
    {
        return glm::normalize(glm::cross(GetRightDirection(), GetForwardDirection()));
    }

    glm::vec2 EditorCamera::PanSpeed() const
    {
        float speed = 4.0f;
        float x = std::min(m_viewportWidth / 1000.0f, 2.4f) * speed;
        float y = std::min(m_viewportHeight / 1000.0f, 2.4f) * speed;
        return { x, y };
    }


    void EditorCamera::SetPosition(const glm::vec3& position)
    {
        m_position = position;
        UpdateView();
    }

    void EditorCamera::SetRotation(float pitch, float yaw)
    {
        m_pitch = pitch;
        m_yaw = yaw;
        UpdateView();
    }

    void EditorCamera::SetProjection(float fov, float aspectRatio, float nearClip, float farClip)
    {
        m_FOV = fov;
        m_aspectRatio = aspectRatio;
        m_nearClip = nearClip;
        m_farClip = farClip;
        UpdateProjection();
    }

    void EditorCamera::SetViewportSize(float width, float height)
    {
        m_viewportWidth = width;
        m_viewportHeight = height;
        m_aspectRatio = width / height;
        UpdateProjection();
    }
}