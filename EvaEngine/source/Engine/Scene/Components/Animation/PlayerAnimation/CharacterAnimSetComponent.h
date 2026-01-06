#pragma once
#include <Engine/Core/Core.h>

struct CharacterAnimSetComponent
{
    // Base locomotion
    AnimClipId idle = INVALID_CLIP;
    AnimClipId run = INVALID_CLIP;
    AnimClipId walk = INVALID_CLIP;

    AnimClipId strafeL = INVALID_CLIP;
    AnimClipId strafeR = INVALID_CLIP;

    // Upper body
    AnimClipId aimIdle = INVALID_CLIP;   // can be INVALID if you don't have it
    AnimClipId fireRifle = INVALID_CLIP; // playerAnimShootRifle
    AnimClipId reload = INVALID_CLIP;    // optional

    // Tunables
    float baseFade = 10.0f;     // how fast base crossfade changes
    float overlayFadeIn = 12.0f;
    float overlayFadeOut = 14.0f;
};
