#pragma once
#include <Engine/UI/UIContext.h>

#include <Engine/UI/Font.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Components/UI/HUDStateComponent.h>
#include "Engine/Scene/Entity.h"
#include "HUDWidgets.h"

class HUDSystem
{
public:
    void Init(Engine::UIContext& ui, const Engine::Ref<Engine::Font>& font);


    void Update(Engine::Ref<Engine::Scene> scene);

private:
    void ApplyHUDState(const HUDStateComponent& hud);

    Engine::Ref<Engine::VulkanTexture> GetWeaponIcon(WeaponType t);
    

    std::string_view WeaponTypeToString(WeaponType t);

private:
    bool m_initialized = false;
    Engine::UIContext* m_ui = nullptr;
    HUDWidgets m_widgets;
};


