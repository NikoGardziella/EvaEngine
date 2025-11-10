#pragma once

#include <cstdint>
#include <Engine/Scene/Components/Animation/AnimationComponent.h>

namespace Engine {

    class Scene;
    class AnimationBank2D;

    class AnimationSystem2D {
    public:
        explicit  AnimationSystem2D(AnimationBank2D& bank) : m_bank(&bank) {}

        void Update(float dt, Scene* scene);


    
        // pick direction by mode (velocity, aim, manual)
        static Dir8 SelectDirection(const AnimationComponent& anim,
            const glm::vec2& vel,  float aimRadians);

        // advance one animator
        static void Advance(AnimatorState& st, const AnimationClipGrid& grid, float dt);

    private:
        AnimationBank2D* m_bank = nullptr;
    };

}
