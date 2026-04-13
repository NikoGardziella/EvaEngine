#include "pch.h"
#include "EditorCamera.h"

#include "Engine/Core/Input.h"
#include <Engine/Events/MouseEvent.h>


#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <Engine/Core/Core.h>

namespace Engine {

    EditorCamera::EditorCamera(float fov, float aspectRatio, float nearClip, float farClip)
        : m_FOV(fov), m_aspectRatio(aspectRatio), m_nearClip(nearClip), m_farClip(farClip)
    {
        UpdateProjection();
        UpdateView();
    }

    void EditorCamera::UpdateProjection()
    {
        //m_projection = glm::perspective(glm::radians(m_FOV), m_aspectRatio, m_nearClip, m_farClip);

        m_projection = glm::perspectiveRH_ZO(glm::radians(m_FOV), m_aspectRatio, m_nearClip, m_farClip);

      //  m_projection[1][1] *= -1.0f;            // Vulkan Y flip
       // EE_CORE_INFO("[Cam] After UpdateProjection: P[1][1]={:.6f}", m_projection[1][1]);
    }

    void EditorCamera::UpdateView()
    {
        const float yaw = glm::radians(m_yaw);
        const float pitch = glm::radians(m_pitch);

        // Right-handed: +X right, +Y up, -Z forward (common), or keep +X forward if that’s your convention
        glm::vec3 forward;
        forward.x = cos(yaw) * cos(pitch);
        forward.y = sin(pitch);
        forward.z = sin(yaw) * cos(pitch);
        forward = glm::normalize(forward);

        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
        glm::vec3 up = glm::normalize(glm::cross(right, forward));

        // For an orbit camera:
        m_position = m_focalPoint - forward * m_distance;   // use the SAME 'forward'
        m_view = glm::lookAt(m_position, m_position + forward, up);
    }

    void EditorCamera::OnUpdate(Timestep timestep)
    {

        if (Input::IsKeyPressed(Key::LeftAlt))
        {
            const glm::vec2 mouse{ Input::GetMouseX(), Input::GetMouseY() };
            glm::vec2 delta = (mouse - m_initialMousePosition) * 0.003f;
            m_initialMousePosition = mouse;

            if (Input::IsMouseButtonPressed(Mouse::ButtonLeft))
            {
                OnMouseRotate(delta);
            }
            else if (Input::IsMouseButtonPressed(Mouse::ButtonRight))
            {
                OnMouseZoom(delta.y);
            }
        }

        if (Input::IsMouseButtonPressed(Mouse::ButtonMiddle))
        {
            const glm::vec2 mouse{ Input::GetMouseX(), Input::GetMouseY() };

            // Reset initial position if the mouse pan just started
            if (!m_isPanning)
            {
                m_initialMousePosition = mouse;
                m_isPanning = true;
            }

            glm::vec2 delta = (mouse - m_initialMousePosition) * 0.003f;
            m_initialMousePosition = mouse;
            OnMousePan(delta);
        }
        else
        {
            m_isPanning = false;  // Reset flag when mouse is released
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
        

		float scrollSpeed = 3.0f;
		if (Input::IsKeyPressed(Key::LeftControl))
		{
			scrollSpeed = 10.0f;
		}


        float delta = event.GetYOffset() * scrollSpeed;
        OnMouseZoom(delta);
        UpdateView();
        return true;
    }

    
    
    void EditorCamera::OnMouseZoom(float deltaY)
    {
        static const float minFOV = 10.0f;   // Minimum zoom-in limit
        static const float maxFOV = 170.0f;  // Maximum zoom-out limit
        static const float zoomSpeed = 0.5f; // Sensitivity for smoother zooming

        m_FOV -= deltaY * zoomSpeed;

        // Clamp the FOV within min and max limits
        if (m_FOV < minFOV) m_FOV = minFOV;
        if (m_FOV > maxFOV) m_FOV = maxFOV;

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

    glm::vec2 EditorCamera::GetScreenCenterWorld(float worldZ) const
    {
        const glm::vec2 center = {
            m_viewportWidth * 0.5f,
            m_viewportHeight * 0.5f
        };

        return ScreenToWorld2D(center, worldZ);
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
        float speed = 15.0f;

        if (Input::IsKeyPressed(Key::LeftControl))
        {
            speed = 45.0f;
        }

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

    glm::vec3 EditorCamera::ScreenToWorld(const glm::vec2& screenPos, float depthNdc) const
    {
        // screenPos is in viewport-local pixels: (0,0) top-left, (width,height) bottom-right

        const float x = (2.0f * screenPos.x) / m_viewportWidth - 1.0f;
        const float y = 1.0f - (2.0f * screenPos.y) / m_viewportHeight;

        glm::vec4 clipPos(x, y, depthNdc, 1.0f);

        glm::mat4 invVP = glm::inverse(GetViewProjection());
        glm::vec4 world = invVP * clipPos;

        if (world.w != 0.0f)
            world /= world.w;

        return glm::vec3(world);
    }

    glm::vec2 EditorCamera::ScreenToWorld2D(const glm::vec2& screenPos, float worldZ) const
    {
        // Unproject near and far points, then intersect ray with z = worldZ plane
        glm::vec3 nearWorld = ScreenToWorld(screenPos, -1.0f);
        glm::vec3 farWorld = ScreenToWorld(screenPos, 1.0f);

        glm::vec3 dir = farWorld - nearWorld;

        if (glm::abs(dir.z) < 1e-6f)
            return glm::vec2(nearWorld.x, nearWorld.y);

        float t = (worldZ - nearWorld.z) / dir.z;
        glm::vec3 hit = nearWorld + dir * t;

        return glm::vec2(hit.x, hit.y);
    }

    void EditorCamera::GetViewportWorldBounds2D(glm::vec2& outMin, glm::vec2& outMax, float worldZ) const
    {
        const glm::vec2 topLeft = ScreenToWorld2D({ 0.0f, 0.0f }, worldZ);
        const glm::vec2 topRight = ScreenToWorld2D({ m_viewportWidth, 0.0f }, worldZ);
        const glm::vec2 bottomLeft = ScreenToWorld2D({ 0.0f, m_viewportHeight }, worldZ);
        const glm::vec2 bottomRight = ScreenToWorld2D({ m_viewportWidth, m_viewportHeight }, worldZ);

        outMin.x = std::min(std::min(topLeft.x, topRight.x), std::min(bottomLeft.x, bottomRight.x));
        outMin.y = std::min(std::min(topLeft.y, topRight.y), std::min(bottomLeft.y, bottomRight.y));

        outMax.x = std::max(std::max(topLeft.x, topRight.x), std::max(bottomLeft.x, bottomRight.x));
        outMax.y = std::max(std::max(topLeft.y, topRight.y), std::max(bottomLeft.y, bottomRight.y));
    }


}