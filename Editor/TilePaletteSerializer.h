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
		TilePaletteSerializer(const std::vector<std::string>& tileNames,
			const std::unordered_map<std::string, glm::vec4>& tileUVMap);

		void Serialize(const std::string& filepath);
		bool Deserialize(const std::string& filepath);

		const std::vector<std::string>& GetTileNames() const { return m_tileNames; }
		const std::unordered_map<std::string, glm::vec4>& GetTileUVMap() const { return m_tileUVMap; }

	private:
		std::vector<std::string> m_tileNames;
		std::unordered_map<std::string, glm::vec4> m_tileUVMap;

	};

};

