#pragma once
#include <Engine/Scene/Components/Combat/WeaponComponent.h>
#include <cstdint>



    struct HUDStateComponent
    {
        // Weapon
        WeaponType weaponType = WeaponType::Pistol;

        int32_t ammoInMag = 0;
        int32_t magSize = 0;
        bool showAmmo = true;

        int32_t ammoReserve = 0;
        bool showCrosshair = true;

       
    };

