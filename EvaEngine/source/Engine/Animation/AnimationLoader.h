#pragma once
#include <string>
#include <cstdint>

namespace Engine {

    class AnimationBank;

    class AnimationLoader {
    public:
        static uint32_t LoadGridYaml(AnimationBank& bank, const std::string& yamlPath);
        
    };

} 
