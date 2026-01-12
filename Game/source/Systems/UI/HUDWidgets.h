#pragma once
#include <Engine/UI/UIElement.h>
#include <Engine/UI/UITextElement .h>
#include <Engine/UI/UIContext.h>
#include <Engine/UI/Font.h>

struct HUDWidgets
{
    Engine::UIImageElement* weaponIcon = nullptr;
    Engine::UITextElement* ammoText = nullptr;
    Engine::UITextElement* weaponName = nullptr;

    void Create(Engine::UIContext& ui, const Engine::Ref<Engine::Font>& font)
    {
        weaponIcon = &ui.Add<Engine::UIImageElement>();
        weaponIcon->tr.anchor = UIAnchorPreset::BottomRight;
        weaponIcon->tr.pivot = { 1.0f, 1.0f };
        weaponIcon->tr.posPx = { -24.0f, -24.0f };
        weaponIcon->tr.sizePx = { 160.0f, 100.0f };
        weaponIcon->layer = 100;

        ammoText = &ui.Add<Engine::UITextElement>();
        ammoText->font = font;
        ammoText->tr.anchor = UIAnchorPreset::BottomRight;
        ammoText->tr.pivot = { 1.0f, 1.0f };
        ammoText->tr.posPx = { -24.0f, -24.0f - 10.0f };
        ammoText->layer = 101;

        weaponName = &ui.Add<Engine::UITextElement>();
        weaponName->font = font;
        weaponName->tr.anchor = UIAnchorPreset::BottomRight;
        weaponName->tr.pivot = { 1.0f, 1.0f };
        weaponName->tr.posPx = { -24.0f, -24.0f - 42.0f };
        weaponName->layer = 102;
    }
};
