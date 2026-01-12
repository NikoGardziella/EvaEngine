#include "pch.h"
#include "HUDSystem.h"
#include <Engine/UI/Font.h>
#include <Engine/AssetManager/AssetManager.h>

void HUDSystem::Init(Engine::UIContext& ui, const Engine::Ref<Engine::Font>& font)
{

	m_ui = &ui;
	m_widgets.Create(ui, font);
	m_initialized = true;
}

void HUDSystem::Update(Engine::Scene* scene)
{
    if (!m_initialized) return;

    scene->ForEach<HUDStateComponent>(
        [&](Engine::Entity e, HUDStateComponent& hud)
        {
         

            ApplyHUDState(hud);
        });
}

void HUDSystem::ApplyHUDState(const HUDStateComponent& hud)
{
    // icon + weapon name
    if (m_widgets.weaponIcon)
        m_widgets.weaponIcon->texture = GetWeaponIcon(hud.weaponType);

    if (m_widgets.weaponName)
        m_widgets.weaponName->text = std::string(WeaponTypeToString(hud.weaponType));

    // ammo
    if (m_widgets.ammoText)
    {
        m_widgets.ammoText->visible = hud.showAmmo;
        if (hud.showAmmo)
            m_widgets.ammoText->text = std::to_string(hud.ammoInMag) + "/" + std::to_string(hud.magSize);
    }
}

Engine::Ref<Engine::VulkanTexture> HUDSystem::GetWeaponIcon(WeaponType t)
{
    switch (t)
    {
    case WeaponType::Grenade:    return Engine::AssetManager::GetTexture("ui_weapon_grenade");
    case WeaponType::Bazooka:    return Engine::AssetManager::GetTexture("ui_weapon_bazooka");
    case WeaponType::MachineGun: return Engine::AssetManager::GetTexture("ui_weapon_rifle");
    case WeaponType::Shotgun:    return Engine::AssetManager::GetTexture("ui_weapon_shotgun");
    case WeaponType::Melee:      return Engine::AssetManager::GetTexture("ui_weapon_melee");
    default:                     return Engine::AssetManager::GetTexture("ui_weapon_pistol");
    }
}

std::string_view HUDSystem::WeaponTypeToString(WeaponType t)
{
    switch (t)
    {
    case WeaponType::Melee:      return "MELEE";
    case WeaponType::Pistol:     return "PISTOL";
    case WeaponType::Shotgun:    return "SHOTGUN";
    case WeaponType::MachineGun: return "RIFLE";
    case WeaponType::Grenade:    return "GRENADE";
    case WeaponType::Bazooka:    return "BAZOOKA";
    }
    return "WEAPON";
}
