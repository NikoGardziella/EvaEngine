#include "pch.h"

#include "RenderCommand.h"
#include "Engine/Platform/OpenGl/OpenGLRendererAPI.h"


namespace Engine {

	RendererAPI* RenderCommand::s_RendererAPI = new OpenGLRendererAPI;



}