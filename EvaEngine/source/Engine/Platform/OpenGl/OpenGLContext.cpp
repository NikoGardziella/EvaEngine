#include "pch.h"
#include "OpenGLContext.h"

#include "GLFW/glfw3.h"
#include <glad/glad.h>
#include <Engine/Core/Log.h>


namespace Engine {


	OpenGLContext::OpenGLContext(GLFWwindow* windowHandle)
		: m_windowHandle(windowHandle)
	{
		if (!windowHandle)
		{
			EE_CORE_ERROR("Failed to initialize {0} with error code {0}", "windowHandle", static_cast<void*>(m_windowHandle));
		}
		else
		{

		}
	}

	void OpenGLContext::Init()
	{
		EE_PROFILE_FUNCTION();


		glfwMakeContextCurrent(m_windowHandle);

		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		if (status == 0)
		{
			EE_CORE_ERROR("Failed to initialize {0} with status code {1} ", "glad", status);
		}
		else
		{
			EE_CORE_INFO(" {0} initialized with status code {1} ", "glad", status);
		}

		EE_CORE_INFO("OpenGL renderer: {0} , {1}",
			reinterpret_cast<const char*>(glGetString(GL_VENDOR)),
			reinterpret_cast<const char*>(glGetString(GL_RENDERER))
		);
	}

	void OpenGLContext::SwapBuffers()
	{
		glfwSwapBuffers(m_windowHandle);

	}

	
}