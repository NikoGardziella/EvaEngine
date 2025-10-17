#pragma once


struct WeaponComponent
{
    float       Damage = 10.0f;
    float       FireRate = 1.0f;
    float       Cooldown = 0.0f;
    float       ProjectileSpeed = 20.0f;
    int         DestructionRadius = TILE_PIXEL_WIDTH / 15;
    bool        IsFiring = false;

    WeaponComponent() = default;
    WeaponComponent(const WeaponComponent&) = default;
};
