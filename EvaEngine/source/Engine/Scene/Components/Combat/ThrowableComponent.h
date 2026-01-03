#pragma once
#include <cstdint>
#include "glm/glm.hpp"

enum class ThrowableType : uint8_t
{
    Grenade,
    Molotov,
    Knife,
    Custom
};

struct ThrowableComponent
{
    uint32_t renderSlot = 0;

    ThrowableType Type = ThrowableType::Grenade;
    glm::vec2 GroundPosWS = glm::vec2(0.0f);
    glm::vec2 VelocityWS = glm::vec2(0.0f);

    float DestructionRadius = 0.1f;
    uint32_t Damge = 1;

    float FuseTime = 0.0f;
    float FuseTimer = 0.0f;
    bool  Triggered = false;

    float Bounciness = 0.4f;
    float MinSpeedToStop = 1.0f;
    int   MaxBounces = 4;
    int   BounceCount = 0;
    bool  ExplodeOnImpact = false;

    // Arc params
    float ArcT = 0.0f;
    float ArcDuration = 0.7f;
    float ArcHeightWorld = 1.0f;

    float InitialSpeed = 0.0f;
    float AirDrag = 0.0f;

    // NEW: how high it starts above ground (visual only)
    float InitialLift = 0.0f;  // in world units

    int   GroundBounceCount = 0;
    int   MaxGroundBounces = 2;

    bool  Initialized = false;

    float RotationZ = 0.0f;       // current angle in radians
    float AngularSpeedZ = 0.0f;   // radians per second

    float MinSpinSpeed = 4.0f;    // tweak these to taste
    float MaxSpinSpeed = 14.0f;
};
