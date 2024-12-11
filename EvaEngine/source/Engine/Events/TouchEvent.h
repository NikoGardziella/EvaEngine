#pragma once
#include "Event.h"

#include <sstream> // testing


namespace Engine {

    enum class TouchPointCode
    {
        None = 0,
        Point1, // Finger 1
        Point2, // Finger 2
        // Add more points if needed
    };

    class TouchEvent : public Event
    {
    public:
        TouchPointCode GetTouchPoint() const { return m_TouchPoint; }
        float GetX() const { return m_X; }
        float GetY() const { return m_Y; }

        EVENT_CLASS_CATEGORY(EventCategoryInput)

    protected:
        TouchEvent(TouchPointCode touchPoint, float x, float y)
            : m_TouchPoint(touchPoint), m_X(x), m_Y(y) {}

        TouchPointCode m_TouchPoint;
        float m_X, m_Y;
    };

    class TouchPressedEvent : public TouchEvent
    {
    public:
        TouchPressedEvent(TouchPointCode touchPoint, float x, float y)
            : TouchEvent(touchPoint, x, y) {}

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "TouchPressedEvent: Point " << static_cast<int>(m_TouchPoint) << " (" << m_X << ", " << m_Y << ")";
            return ss.str();
        }

        EVENT_CLASS_TYPE(TouchPressed)
    };

    class TouchReleasedEvent : public TouchEvent
    {
    public:
        TouchReleasedEvent(TouchPointCode touchPoint, float x, float y)
            : TouchEvent(touchPoint, x, y) {}

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "TouchReleasedEvent: Point " << static_cast<int>(m_TouchPoint) << " (" << m_X << ", " << m_Y << ")";
            return ss.str();
        }

        EVENT_CLASS_TYPE(TouchReleased)
    };

    class TouchMovedEvent : public TouchEvent
    {
    public:
        TouchMovedEvent(TouchPointCode touchPoint, float x, float y)
            : TouchEvent(touchPoint, x, y) {}

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "TouchMovedEvent: Point " << static_cast<int>(m_TouchPoint) << " (" << m_X << ", " << m_Y << ")";
            return ss.str();
        }

        EVENT_CLASS_TYPE(TouchMoved)
    };

} // namespace Engine
