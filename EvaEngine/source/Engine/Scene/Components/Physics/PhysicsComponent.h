#pragma once

#include "glm/glm.hpp"
struct PhysicsComponent {
    // Config
    float duration = 0.7f;      // seconds to simulate
    glm::vec2 gravity = { 0.0f, -9.81f }; // Y-up world; flip sign if Y-down
    float linearDamping = 0.10f;     // 0..1 per second (exponential)
    float angularDamping = 0.10f;

    // State
    glm::vec2 velocity = { 0.0f, 0.0f };
    float angularVelocity = 0.0f;    // rad/s
    float timeLeft = 0.0f;

    // Optional: when finished, destroy or just freeze
    bool destroyOnFinish = false;
    bool removeOnFinish = false;
    bool active = false;    // internal: running or not
    bool  randomizedSpin = false;

};
