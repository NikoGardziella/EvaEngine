#pragma once
#include <glm/ext/vector_float2.hpp>


struct CharacterControllerComponent
{
    float speed = 5.0f;
    glm::vec2 velocity = { 0.0f, 0.0f };
    bool onGround = false;



    CharacterControllerComponent() = default;
    CharacterControllerComponent(const CharacterControllerComponent&) = default;
};