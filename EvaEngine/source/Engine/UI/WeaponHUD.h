#pragma once
#include "UIElement.h"
#include "UIContext.h"

namespace Engine {

    struct WeaponHUD
    {
        UIImageElement* icon = nullptr;
        // UITextElement* ammo = nullptr; // later when text exists

        void Create(UIContext& ui)
        {
            auto& iconEl = ui.Add<UIImageElement>();
            iconEl.tr.anchor = UIAnchorPreset::TopLeft;
            iconEl.tr.pivot = { 0.5f, 0.5f };
            iconEl.tr.posPx = { 0.0f, 0.0f };
            iconEl.tr.sizePx = { 80, 80 };
            iconEl.tr.layer = 100;
            icon = &iconEl;
        }

        void SetWeaponIcon(const Ref<VulkanTexture>& tex)
        {
            if (icon) icon->texture = tex;
        }
    };
}
