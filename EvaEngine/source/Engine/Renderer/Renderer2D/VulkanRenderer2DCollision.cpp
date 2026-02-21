#include "pch.h"
#include "VulkanRenderer2D.h"
#include <Engine/Events/Public/CollisionEvents.h>

namespace Engine {

	void VulkanRenderer2D::CalculateBoxCollision(const glm::vec2& position, const glm::vec2& size,
		float rotation, uint64_t entityID, eCollisionType collisionType, uint32_t damage)
	{
		EE_PROFILE_FUNCTION();

		uint32_t index = s_CollisionData.EntitySlotIndex;
		if (index >= MAX_COLLISION_ENTITIES)
		{
			EE_CORE_WARN("Max vehicle collision slots reached!");
			return;
		}
		s_CollisionData.CollisionEntities[index].Type = (uint32_t)collisionType;
		s_CollisionData.CollisionEntities[s_CollisionData.EntitySlotIndex].Damage = damage;

		s_CollisionData.CollisionEntities[index].Position = position;
		s_CollisionData.CollisionEntities[index].Size = size;
		s_CollisionData.CollisionEntities[index].Rotation = rotation;
		s_CollisionData.CollisionEntities[index].ID_Low = static_cast<uint32_t>(entityID & 0xFFFFFFFF);
		s_CollisionData.CollisionEntities[index].ID_High = static_cast<uint32_t>(entityID >> 32);
		s_CollisionData.EntitySlotIndex++;
	}



	void VulkanRenderer2D::CalculateCircleCollision(const glm::vec2& colliderPos, const float colliderRadius, uint64_t entityID,
		eCollisionType collisionType, uint32_t damage, const float destructionRadius, glm::vec2  projectileDirection,
		glm::vec2  TargetPositionAtFireTime, float  DistanceToTargetatFireTime, float  TargetPositionHeightZ1)
	{
		EE_PROFILE_FUNCTION();

		uint32_t index = s_CollisionData.EntitySlotIndex;
		if (index >= MAX_COLLISION_ENTITIES)
		{
			EE_CORE_WARN("Max collision slots reached!");
			return;
		}

		s_CollisionData.CollisionEntities[index].Type = (uint32_t)collisionType;
		s_CollisionData.CollisionEntities[s_CollisionData.EntitySlotIndex].Position = colliderPos;
		s_CollisionData.CollisionEntities[s_CollisionData.EntitySlotIndex].Damage = damage;
		s_CollisionData.CollisionEntities[s_CollisionData.EntitySlotIndex].DestructionRadius = destructionRadius;
		s_CollisionData.CollisionEntities[s_CollisionData.EntitySlotIndex].ColliderRadius = colliderRadius;
		s_CollisionData.CollisionEntities[s_CollisionData.EntitySlotIndex].Dir = projectileDirection;
		s_CollisionData.CollisionEntities[s_CollisionData.EntitySlotIndex].EndPos = TargetPositionAtFireTime;
		s_CollisionData.CollisionEntities[s_CollisionData.EntitySlotIndex].RayLen = DistanceToTargetatFireTime;
		s_CollisionData.CollisionEntities[s_CollisionData.EntitySlotIndex].Z1 = TargetPositionHeightZ1;

		s_CollisionData.CollisionEntities[s_CollisionData.EntitySlotIndex].ID_Low = static_cast<uint32_t>(entityID & 0xFFFFFFFF);
		s_CollisionData.CollisionEntities[s_CollisionData.EntitySlotIndex].ID_High = static_cast<uint32_t>(entityID >> 32);
		s_CollisionData.EntitySlotIndex++;
	}

	void VulkanRenderer2D::CalculatePlayerCircleCollision(const glm::vec2& colliderPos, const float colliderRadius, uint64_t entityID, eCollisionType collisionType)
	{
		EE_PROFILE_FUNCTION();

		uint32_t playerIndex = 0; // only one player

		s_CollisionData.playerEntities[playerIndex].Position = colliderPos;
		s_CollisionData.playerEntities[playerIndex].ColliderRadius = colliderRadius;
		s_CollisionData.playerEntities[playerIndex].ID_Low = static_cast<uint32_t>(entityID & 0xFFFFFFFF);
		s_CollisionData.playerEntities[playerIndex].ID_High = static_cast<uint32_t>(entityID >> 32);
	}


	void VulkanRenderer2D::ReadAndResetCollisionBuffer(uint32_t currentFrame)
	{
		EE_PROFILE_FUNCTION();

		const uint32_t readIdx = (currentFrame + MAX_FRAMES_IN_FLIGHT - 1) % MAX_FRAMES_IN_FLIGHT;
		const uint32_t writeIdx = currentFrame;

		vkWaitForFences(m_device, 1, &m_inFlightFences[readIdx], VK_TRUE, UINT64_MAX);

		CollisionResultBuffer collisionResult{};
		{
			VkMappedMemoryRange inv{ VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
			inv.memory = m_vulkanGraphicsPipelines->GetGPUCollisionMemory(readIdx);
			inv.offset = 0;
			inv.size = sizeof(CollisionResultBuffer);
			vkInvalidateMappedMemoryRanges(m_device, 1, &inv);

			std::memcpy(&collisionResult, m_collisionMapped[readIdx], sizeof(CollisionResultBuffer));
		}


		// 2) Convert to CPU vectors
		CollisionResultsCPU::LatestProjectiles.clear();
		m_hitsW.clear();
		m_radiiW.clear();
		m_damages.clear();
		s_CPUExplosionsData.CPUExplosions.clear();

		{
			const uint32_t count = std::min(collisionResult.collisionCount, (uint32_t)MAX_COLLISION_RESULTS);
			for (uint32_t i = 0; i < count; ++i)
			{
				const auto& r = collisionResult.results[i];

				if (r.collisionDetected == 0xFFFFFFFFu)
				{
					continue;
				}
				EE_CORE_INFO("collision at world {} | {}", r.CollisionPosition.x, r.CollisionPosition.y);

				// for effects pass
				m_hitsW.push_back(r.CollisionPosition);
				m_radiiW.push_back(r.DestructionRadius);
				m_damages.push_back(r.Damage);

				// for projectilSystem, grid, etc.
				Collision coll{};
				coll.EntityID = (uint64_t(r.hitProjectileID_High) << 32) | uint64_t(r.hitProjectileID_Low);
				coll.HitPosition = r.CollisionPosition;
				coll.Health = r.Health;
				coll.RadiusWS = r.DestructionRadius;
				CollisionResultsCPU::LatestProjectiles.push_back(coll);
			}
		}

		// 3) Reset writeIdx buffer for this frame’s compute
		{
			CollisionResultBuffer* buf = m_collisionMapped[writeIdx];

			// Clear the whole struct once (like before), but using the mapped pointer.
			std::memset(buf, 0xFF, sizeof(CollisionResultBuffer));
			buf->collisionCount = 0; // override if you don't want 0xFFFFFFFF here

			VkMappedMemoryRange fl{ VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
			fl.memory = m_vulkanGraphicsPipelines->GetGPUCollisionMemory(writeIdx);
			fl.offset = 0;
			fl.size = VK_WHOLE_SIZE;
			vkFlushMappedMemoryRanges(m_device, 1, &fl);
		}
	}



	void VulkanRenderer2D::CalculateCollisionFrame(uint32_t currentFrame, VkCommandBuffer cmd)
	{
		EE_PROFILE_FUNCTION();




		vkResetCommandBuffer(cmd, 0);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(cmd, &beginInfo);





		{
			//*********** update descriptor sets ***********
			m_vulkanGraphicsPipelines->UpdateCollisionUniformBuffer(currentFrame, s_CollisionData.CollisionEntities);

			// combine this to the next

			// 1) Bind buffers (results/projectiles/mask) for this frame
			VkBuffer blockedMaskBuffer = m_vulkanGraphicsPipelines->GetBlockedTileMaskBuffer();

			VkDeviceSize blockedMaskBufferSize = Engine::VulkanGraphicsPipeline::DIRTYOUT_TOTAL;

			VkBuffer collisionResultBuffer = m_vulkanGraphicsPipelines->GetGPUCollisionResultBuffer(currentFrame);
			VkDeviceSize collisionResultBufferSize = sizeof(CollisionResultBuffer);

			VkBuffer projectileBuffer = m_vulkanGraphicsPipelines->GetBulletUniformBuffer(currentFrame).GetBuffer();
			VkDeviceSize projectileBufferSize = m_vulkanGraphicsPipelines->GetBulletUniformBuffer(currentFrame).m_size;

			s_bindlessDescitproRenderer->ComputeBindBuffers(currentFrame,
				collisionResultBuffer, collisionResultBufferSize,
				projectileBuffer, projectileBufferSize,
				blockedMaskBuffer, blockedMaskBufferSize);



			// this could be more explicit 
			const uint32_t readIdx = (currentFrame + MAX_FRAMES_IN_FLIGHT - 1) % MAX_FRAMES_IN_FLIGHT;
			VkBuffer previousFrameCollisionResultBuffer = m_vulkanGraphicsPipelines->GetGPUCollisionResultBuffer(readIdx);

			s_bindlessDescitproRenderer->UpdateEffectImageDescriptorSets(currentFrame, s_VulkanData.VisualEffectsTextureSlots);

			s_bindlessDescitproRenderer->EffectsBindBuffers(currentFrame,
				previousFrameCollisionResultBuffer, collisionResultBufferSize,
				projectileBuffer, projectileBufferSize,
				blockedMaskBuffer, blockedMaskBufferSize);


		}

		RecordComputeCommandBuffer(cmd, currentFrame);
		RecordEffectComputeCommandBuffer(cmd, currentFrame);

		{
			EE_PROFILE_SCOPE("blocked tiles buffer");

			//ProcessDirtyOutThisFrame();
		}


	}


}