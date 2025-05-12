#pragma once

namespace Engine {

    struct HealthComponent
    {
        float Current = 100.0f;
        float Max = 100.0f;

        HealthComponent() = default;
        HealthComponent(const HealthComponent&) = default;
        HealthComponent(float health)
            : Max(health) 
        {
		    Current = health;
        }

    };


}