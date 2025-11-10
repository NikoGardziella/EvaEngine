#pragma once
#include <string>
#include <cstdint>

namespace Engine {

    class AnimationBank2D;

    class AnimationLoader2D {
    public:
        static uint32_t Load2DGridYaml(AnimationBank2D& bank, const std::string& yamlPath);
        
    };

} 
