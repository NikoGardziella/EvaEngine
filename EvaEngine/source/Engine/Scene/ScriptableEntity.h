#pragma once

#include "Engine/Scene/Entity.h"
#include "Engine/Core/Timestep.h"

namespace Engine {

	class ScriptableEntity
	{
	public:

		virtual ~ScriptableEntity() = default;

		template<typename T>
		T& GetComponent()
		{
			return m_entity.GetComponent<T>();
		}


	protected:
		virtual void OnCreate() {}
		virtual void OnDestroy() {}
		virtual void OnUpdate(Timestep ts) {}

	private:
		Entity m_entity;
		friend class Scene;
	};
}

