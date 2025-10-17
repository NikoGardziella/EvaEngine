#pragma once


struct WeaponComponent
{
    uint32_t    Damage = 1;
    float       FireRate = 1.0f;
    float       Cooldown = 0.0f;
    float       ProjectileSpeed = 20.0f;
    float       DestructionRadius = 0.1f; // world tile 
    bool        IsFiring = false;

    WeaponComponent() = default;
    WeaponComponent(const WeaponComponent&) = default;
};
