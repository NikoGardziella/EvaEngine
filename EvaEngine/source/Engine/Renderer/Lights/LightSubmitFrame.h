#pragma once
#include <cstdint>
#include <vector>
#include "GPULights.h"

namespace Engine {


    struct LightSubmitFrame
    {
        std::vector<GPUPointLight> points;
        std::vector<GPUSpotLight>  spots;
        std::vector<GPUDirectionalLight> dirs;
    };


}
