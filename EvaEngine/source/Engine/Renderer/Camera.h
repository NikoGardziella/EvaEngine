#pragma once
#include <glm/glm.hpp>



namespace Engine {


	class Camera
	{
	public:
		Camera() = default;
		Camera(const glm::mat4& projection)
			: m_projection(projection)
		{

		}
		virtual ~Camera() = default;
		const glm::mat4& GetViewProjection() const { return m_projection; }



	// let derived Cameras acess
	protected:

		glm::mat4 m_projection = glm::mat4(1.0f);
	};
}