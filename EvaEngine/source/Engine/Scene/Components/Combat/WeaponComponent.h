#pragma once

enum class WeaponType : uint8_t
{
    Melee,
    Pistol,
    Shotgun,
    MachineGun,
    Grenade,
    Bazooka
};


struct WeaponComponent
{
    WeaponType type = WeaponType::Pistol;

    uint32_t Damage = 1;

    float FireRate = 1.0f;  // seconds between shots
    float Cooldown = 0.0f;
    float ProjectileSpeed = 20.0f;
    float DestructionRadius = 0.1f; // world tile

    // Extra weapon-specific parameters:
    float MaxRange = 20.0f;
    float SpreadDegrees = 0.0f;  // shotgun/bazooka random cone
    uint32_t Pellets = 1;     // shotgun: >1
    bool Automatic = false; // machine gun true, pistol false

    // Melee:
    float MeleeRange = 1.5f;
    float MeleeArcDegrees = 90.0f;

    // Explosive:
    float ExplosionRadius = 2.0f;  // grenade/bazooka
    bool  Explosive = false;

    bool IsFiring = false;

    WeaponComponent() = default;
    WeaponComponent(const WeaponComponent&) = default;
};
