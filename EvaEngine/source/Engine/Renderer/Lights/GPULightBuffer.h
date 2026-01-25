#pragma once
#include "GPULights.h"
#include <Engine/Core/Config.h>

namespace Engine {

    struct GPULightBuffer
    {
        GPULightHeader header;
        GPUDirectionalLight dir[MAX_DIR_LIGHTS];
        GPUPointLight       point[MAX_POINT_LIGHTS];
        GPUSpotLight        spot[MAX_SPOT_LIGHTS];
    };
}
