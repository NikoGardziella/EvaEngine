#pragma once
#include <Engine/Platform/Vulkan/VulkanGraphicsPipeline.h>

namespace Engine
{

	struct Collision
	{
		uint64_t EntityID = 0;
		uint32_t Health = 0;
		int TargetID = 0;
		glm::vec2 HitPosition = { 0.0f, 0.0f };
		glm::vec2 HitNormal = { 0.0f, 0.0f };
		void Reset()
		{
			EntityID = 0;
			TargetID = 0;
			HitPosition = { 0.0f, 0.0f };
			HitNormal = { 0.0f, 0.0f };
		}
		bool IsValid() const
		{
			return EntityID > 0 && TargetID > 0;
		}
		uint64_t GetEntityID() const { return EntityID; }
	};

	
    struct CollisionResultsCPU
    {
        static inline std::vector<Collision> LatestProjectiles;
    };
}