#pragma once
#include <glm/glm.hpp>
#include <Engine/Scene/Components/Environment/DayNightComponent.h>


    class Scene;
    class DayNightSystem
    {
    public:
        static void UpdateDayNightSystem(float deltaTime, Engine::Scene* scene);

    private:


        static void ComputeSunDirection(struct Engine::DayNightComponent& dn);
        static void ComputeLightColors(struct Engine::DayNightComponent& dn);
        static void ApplyToDirectionalLight(Engine::Scene* scene, const struct Engine::DayNightComponent& dn);

    
    };


