#pragma once
#include <cstdint>
#include "Engine/Scene/Components/Projectiles/ProjectileComponent.h"


namespace Engine {

    class ProjectileVisual
    {
    public:
        // Called *after* TileManager::BuildInitialResidency(...)
        static void RegisterVisual(ProjectileVisualType type, uint64_t uid, const uint32_t slot)
        {
            s_registry.RegisterVisual(type, uid, slot);
        }

        static uint32_t GetSlot(ProjectileVisualType type)
        {
            return s_registry.GetSlot(type);
        }

    private:
        struct ProjectileVisualRegistry
        {
            uint32_t slots[(size_t)ProjectileVisualType::Count]{};

            void RegisterVisual(ProjectileVisualType type, uint64_t uid, uint32_t slot)
            {
                slots[(size_t)type] = slot;
            }

            uint32_t GetSlot(ProjectileVisualType type) const
            {
                return slots[(size_t)type];
            }
        };

    public:


        static ProjectileVisualRegistry s_registry;
    };

} 
