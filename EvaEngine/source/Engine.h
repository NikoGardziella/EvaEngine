#pragma once

// ============================================================================
// CORE (always needed)
// ============================================================================
#include "Engine/Core/Core.h"
#include "Engine/Core/Config.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Assert.h"
#include "Engine/Core/Timestep.h"
#include "Engine/Core/Input.h"

// Application framework
#include "Engine/Core/Application.h"
#include "Engine/Core/Layer.h"

// ============================================================================
// THIRD-PARTY (commonly used)
// ============================================================================
#include <glm/glm.hpp>
#include <entt.hpp>

// ============================================================================
// EVENTS
// ============================================================================
#include "Engine/Events/KeyCode.h"
#include "Engine/Events/MouseCodes.h"

// ============================================================================
// SCENE
// ============================================================================
#include "Engine/Scene/Scene.h"
#include "Engine/Scene/Entity.h"
#include "Engine/Scene/ScriptableEntity.h"

// ============================================================================
// RENDERER
// ============================================================================
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/RenderCommand.h"
#include "Engine/Renderer/Renderer2D.h"
#include "Engine/Renderer/Buffer.h"
#include "Engine/Renderer/Framebuffer.h"
#include "Engine/Renderer/Shader.h"
#include "Engine/Renderer/Texture.h"
#include "Engine/Renderer/SubTexture2D.h"
#include "Engine/Renderer/VertexArray.h"
#include "Engine/Renderer/OrthographicCamera.h"
#include "Engine/Renderer/OrthographicCameraController.h"

// ============================================================================
// UI/IMGUI
// ============================================================================
#include "Engine/ImGui/ImGuiLayer.h"