#include <glm/ext/vector_float2.hpp>

#include <Engine/Scene/Entity.h>


    enum eVehicleType
    {
        UnDefined = 0,
        Car = 1,
        Tank = 2,
    };

    struct VehicleComponent
    {
        Engine::Entity Driver = Engine::Entity{};
        glm::vec2 Velocity = glm::vec2(0.0f);

        float MaxSpeed = 15.0f;
        float Power = 2000.0f;
        float Mass = 400.0f;

        float CurrentSpeed = 0.0f;
        float Acceleration = 30.0f;
        float Deceleration = 50.0f;


        float ExitEnterCooldown = 0.5f;
        glm::vec2 Pushback = glm::vec2(0.0f);
    };

