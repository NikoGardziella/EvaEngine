#pragma once
#include "glm/glm.hpp"
#include <string>
#include <vector>
#include <unordered_map>


namespace Engine
{

	class TilePaletteSerializer
	{

    public:
        static void Serialize(const std::string& filepath,
            const std::vector<std::string>& tileNames,
            const std::unordered_map<std::string, glm::vec4>& tileUVMap);

        static bool Deserialize(const std::string& filepath,
            std::vector<std::string>& outTileNames,
            std::unordered_map<std::string, glm::vec4>& outTileUVMap);
	};
}


