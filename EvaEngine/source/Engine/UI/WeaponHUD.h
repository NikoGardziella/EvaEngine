#pragma once
#include "UIElement.h"
#include "UIContext.h"
#include "UITextElement .h"

namespace Engine {

    struct WeaponHUD
    {
        UIImageElement* icon = nullptr;
        UITextElement* ammo = nullptr;  // "12/30"
        UITextElement* name = nullptr;  // "RIFLE" optional

        void Create(UIContext& ui, const Ref<Font>& font)
        {
            icon = &ui.Add<UIImageElement>();
            icon->tr.anchor = UIAnchorPreset::BottomRight;
            icon->tr.pivot = { 1, 1 };
            icon->tr.posPx = { -24, -24 };
            icon->tr.sizePx = { 80, 80 };
            icon->layer = 100;

            ammo = &ui.Add<UITextElement>();
            ammo->font = font;
            ammo->tr.anchor = UIAnchorPreset::BottomRight;
            ammo->tr.pivot = { 1, 1 };
            ammo->tr.posPx = { -24, -24 - 6 }; // just above/below, tune
            ammo->layer = 101;

            name = &ui.Add<UITextElement>();
            name->font = font;
            name->tr.anchor = UIAnchorPreset::BottomRight;
            name->tr.pivot = { 1, 1 };
            name->tr.posPx = { -24, -24 - 32 };
            name->layer = 102;
        }

        void SetIcon(const Ref<VulkanTexture>& tex)
        {
            if (icon) icon->texture = tex;
        }

        void SetAmmo(int inMag, int magSize)
        {
            if (ammo) ammo->text = std::to_string(inMag) + "/" + std::to_string(magSize);
        }

        void SetName(std::string_view s)
        {
            if (name) name->text = std::string(s);
        }
    };

}
