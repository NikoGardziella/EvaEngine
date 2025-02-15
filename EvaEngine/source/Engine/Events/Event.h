#pragma once
#include "Engine/Core/Core.h"

#include <string>
#include <functional>

#include <spdlog/formatter.h>

namespace Engine {
	enum class EventType
	{
		None = 0,
		WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved,
		AppTick, AppUpdate, AppRender,
		KeyPressed, KeyReleased, KeyTyped,
		MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled,
		TouchPressed, TouchReleased, TouchMoved
	};

	enum EventCategory
	{
		None = 0,
		EventCategoryApplication = BIT(0),
		EventCategoryInput = BIT(1),
		EventCategoryKeyboard = BIT(2),
		EventCategoryMouse = BIT(3),
		EventCategoryMouseButton = BIT(4),
		EventCategoryTouch = BIT(5)
	};



#define EVENT_CLASS_TYPE(type) static EventType GetStaticType() { return EventType::type; }\
								virtual EventType GetEventType() const override { return GetStaticType(); }\
								virtual const char* GetName() const override { return #type; }

#define EVENT_CLASS_CATEGORY(category) virtual int GetCategoryFlags() const override { return category; }


	

	class Event
	{
	public:
		virtual ~Event() = default;

		bool Handled = false;

		virtual EventType GetEventType() const = 0;
		virtual const char* GetName() const = 0;
		virtual int GetCategoryFlags() const = 0;
		virtual std::string ToString() const { return GetName(); }

		bool IsInCategory(EventCategory category)
		{
			return GetCategoryFlags() & category;
		}

		
	};

	

	class EventDispatcher
	{
	public:
		/**
		 * @brief Constructs an EventDispatcher for the given event.
		 * @param event Reference to the event that needs to be dispatched.
		 */
		EventDispatcher(Event& event)
			: m_Event(event)
		{
		}

		/**
		 * @brief Dispatches the event if its type matches the expected type.
		 *
		 * This method uses templates to determine the type of event at compile time
		 * and checks if the given event matches the specified type. If it matches,
		 * the provided callback function (`func`) is executed with the event cast
		 * to the specific type.
		 *
		 * @tparam T The type of the event to dispatch (e.g., KeyPressedEvent).
		 * @tparam F The type of the callback function (deduced automatically).
		 * @param func The callback function to handle the event. Should take an
		 *             argument of type `T&` and return a `bool` indicating if
		 *             the event was handled.
		 * @return true if the event was dispatched and handled; false otherwise.
		 */
		template<typename T, typename F>
		bool Dispatch(const F& func)
		{
			// Check if the type of the current event matches the expected type
			if (m_Event.GetEventType() == T::GetStaticType())
			{
				// Call the callback function with the event, cast to the correct type
				// Combine the return value of the callback with the existing Handled flag
				m_Event.Handled |= func(static_cast<T&>(m_Event));
				return true; // Event was dispatched and handled
			}
			return false; // Event type did not match; not dispatched
		}

	private:
		Event& m_Event; ///< Reference to the event being dispatched.
	};

	inline std::ostream& operator<<(std::ostream& os, const Event& e)
	{
		return os << e.ToString();
	}




}