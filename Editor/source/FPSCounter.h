#pragma once
#include <iostream>
#include <chrono>
#include <Engine/Core/Timestep.h>

namespace Engine {




    class FPSCounter {
    public:
        void Update(Timestep timestep) {
            frameCount++;
            elapsedTime += timestep;

            if (elapsedTime >= 1.0f) { // Every second, update FPS
                fps = frameCount;
                frameCount = 0;
                elapsedTime = 0.0f;
            }
        }

        int GetFPS() const { return fps; }

    private:
        uint32_t fps = 0;
        uint32_t frameCount = 0;
        float elapsedTime = 0.0f;
    };

}