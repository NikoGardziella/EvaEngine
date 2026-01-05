#pragma once
#include <cstdint>
#include "Engine/Scene/Entity.h"
#include "glm/glm.hpp"

struct WeaponDef
{
    uint32_t meshId;
    uint32_t materialId;
    uint16_t attachBoneIndex;
    glm::mat4 localOffset;

    enum class Type { Throwable, Hitscan, Projectile } type;

};

struct WeaponInventoryComponent
{
    uint32_t equippedWeaponDefId = 0;      // points to a weapon definition (grenade, pistol, etc.)
    Engine::Entity equippedWeaponEntity;   // runtime spawned weapon entity (or null)
    bool equipDirty = true;                // set true when switching
};




struct EquippedWeaponComponent
{
    uint32_t meshId = 0xFFFFFFFFu;
    uint32_t materialId = 0xFFFFFFFFu;
    bool visible = true;

    Engine::Entity owner;            // player entity
    uint16_t attachBoneIndex = 0xFFFF;

    // Bone-space offset (grip pose)
    glm::mat4 localOffset = glm::mat4(1.0f);


};

