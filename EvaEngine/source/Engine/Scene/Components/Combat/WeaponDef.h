#pragma once
#include "WeaponComponent.h"
#include "glm/glm.hpp"
#include <glm/gtx/transform.hpp>

namespace WeaponDefinition {


    struct WeaponDef
    {
        const char* meshKey = nullptr;
        WeaponComponent weaponStats{};
        glm::mat4 localOffset = glm::mat4(1.0f);
    };

    static WeaponDef MakeWeaponDef(WeaponType type)
    {
        WeaponDef def{};

        glm::mat4 flipX = glm::rotate(glm::mat4(1.0f), glm::pi<float>(), glm::vec3(1, 0, 0));

        switch (type)
        {
        case WeaponType::MachineGun: // Rifle
        {
            def.meshKey = "ak47";
            def.weaponStats.type = WeaponType::MachineGun;
            def.weaponStats.Damage = 5;
            def.weaponStats.FireRate = 0.08f;
            def.weaponStats.Automatic = true;
            def.weaponStats.ProjectileSpeed = 35.0f;
            def.weaponStats.SpreadDegrees = 1.2f;
            def.weaponStats.DestructionRadius = 0.1f;

            def.weaponStats.Explosive = false;

            def.localOffset = glm::translate(glm::mat4(1.0f), glm::vec3(-0.07f, 0.10f, 0.00f)) * flipX;
        } break;

        case WeaponType::Bazooka:
        {
            def.meshKey = "rocketlaucher";
            def.weaponStats.type = WeaponType::Bazooka;
            def.weaponStats.Damage = 40;
            def.weaponStats.FireRate = 0.8f;
            def.weaponStats.Automatic = false;
            def.weaponStats.ProjectileSpeed = 22.0f;
            def.weaponStats.Explosive = true;
            def.weaponStats.ExplosionRadius = 3.5f;
            def.weaponStats.DestructionRadius = 0.6f;

            def.localOffset =
                glm::translate(glm::mat4(1.0f), glm::vec3(-0.10f, 0.12f, 0.00f)) * flipX;
        } break;

        case WeaponType::Grenade:
        {
            def.meshKey = "nade_low";
            def.weaponStats.type = WeaponType::Grenade;
            def.weaponStats.Damage = 40;
            def.weaponStats.FireRate = 0.0f; 
            def.weaponStats.Explosive = true;
            def.weaponStats.ExplosionRadius = 2.5f;
            def.weaponStats.DestructionRadius = 0.8f;

            // charge params you already have
            def.weaponStats.GrenadeMaxCharge = 1.0f;
            def.weaponStats.GrenadeMinSpeed = 8.0f;
            def.weaponStats.GrenadeMaxSpeed = 25.0f;

            def.localOffset =
                glm::translate(glm::mat4(1.0f), glm::vec3(-0.04f, 0.08f, 0.02f)) * flipX;
        } break;

        default:
        {
            def.meshKey = "ak47";
            def.weaponStats.type = type;
            def.localOffset = glm::translate(glm::mat4(1.0f), glm::vec3(-0.07f, 0.10f, 0.00f)) * flipX;
        } break;
        }

        return def;
    }



}
