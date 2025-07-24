#pragma once
#include <Engine/Scene/Entity.h>

struct DriverComponent
{
    Engine::Entity Vehicle = Engine::Entity{};
   
    DriverComponent() = default;
    DriverComponent(const Engine::Entity& vehicle) : Vehicle(vehicle) {}
};
