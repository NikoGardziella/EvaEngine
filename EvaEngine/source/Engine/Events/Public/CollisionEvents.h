#pragma once
#include <Engine/Platform/Vulkan/VulkanGraphicsPipeline.h>

namespace Engine
{

	struct Collision
	{
		uint64_t ProjectileID = 0;
		int TargetID = 0;
		glm::vec2 HitPosition = { 0.0f, 0.0f };
		glm::vec2 HitNormal = { 0.0f, 0.0f };
		void Reset()
		{
			ProjectileID = 0;
			TargetID = 0;
			HitPosition = { 0.0f, 0.0f };
			HitNormal = { 0.0f, 0.0f };
		}
		bool IsValid() const
		{
			return ProjectileID > 0 && TargetID > 0;
		}
		uint64_t GetProjectileID() const { return ProjectileID; }
	};

	// Supports only one collision result at a time
	// todo add vector of collisions
    struct CollisionResults
    {

        static inline Collision Latest;
    };
}