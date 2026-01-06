#pragma once
#include <cstdint>
#include "Engine/Scene/Entity.h"
#include "glm/glm.hpp"
#include "WeaponComponent.h"

struct WeaponDef
{
    uint32_t meshId;
    uint32_t materialId;
    uint16_t attachBoneIndex;
    glm::mat4 localOffset;

    enum class Type { Throwable, Hitscan, Projectile } type;

};
enum class WeaponSlot : uint8_t
{
    Slot1 = 1,
    Slot2 = 2,
    Slot3 = 3
};

struct WeaponInventoryComponent
{
    // what player wants vs what is currently active
    WeaponSlot desiredSlot = WeaponSlot::Slot1;
    WeaponSlot equippedSlot = WeaponSlot::Slot1;

    bool equipDirty = true;

    Engine::Entity equippedWeaponEntity = {}; // or Engine::Entity::Null()

    // per-slot loadout (simple)
    WeaponType slot1 = WeaponType::MachineGun; // Rifle
    WeaponType slot2 = WeaponType::Bazooka;
    WeaponType slot3 = WeaponType::Grenade;

    // optional: allow per-slot mesh keys / offsets if needed
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

