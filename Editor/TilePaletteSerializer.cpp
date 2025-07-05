#include "TilePaletteSerializer.h"
#include "Core/Log.h"





namespace Engine {

	TilePaletteSerializer::TilePaletteSerializer(const std::vector<std::string>& tileNames,
		const std::unordered_map<std::string, glm::vec4>& tileUVMap)
		: m_tileNames(tileNames), m_tileUVMap(tileUVMap)
	{
	}
	void TilePaletteSerializer::Serialize(const std::string& filepath)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;

		out << YAML::Key << "TilePalette" << YAML::BeginSeq;

		for (const auto& name : m_tileNames)
		{
			const glm::vec4& uv = m_tileUVMap.at(name);
			out << YAML::BeginMap;
			out << YAML::Key << "Name" << YAML::Value << name;
			out << YAML::Key << "UV" << YAML::Value << YAML::Flow << YAML::BeginSeq << uv.x << uv.y << uv.z << uv.w << YAML::EndSeq;
			out << YAML::EndMap;
		}

		out << YAML::EndSeq;
		out << YAML::EndMap;

		std::ofstream fout(filepath);
		if (!fout)
		{
			EE_CORE_ERROR("Failed to open file for writing: {}", filepath);
			return;
		}
		fout << out.c_str();
	}

	bool TilePaletteSerializer::Deserialize(const std::string& filepath)
	{
		YAML::Node data;
		try
		{
			data = YAML::LoadFile(filepath);
		}
		catch (YAML::ParserException& e)
		{
			EE_CORE_ERROR("Failed to load TilePalette YAML file: {}", e.what());
			return false;
		}

		auto palette = data["TilePalette"];
		if (!palette || !palette.IsSequence())
		{
			EE_CORE_ERROR("Invalid TilePalette file structure");
			return false;
		}

		m_tileNames.clear();
		m_tileUVMap.clear();

		for (const auto& tile : palette)
		{
			std::string name = tile["Name"].as<std::string>();
			auto uvNode = tile["UV"];

			if (!uvNode || uvNode.size() != 4)
			{
				EE_CORE_ERROR("Invalid UV for tile '{}'", name);
				continue;
			}

			glm::vec4 uv;
			uv.x = uvNode[0].as<float>();
			uv.y = uvNode[1].as<float>();
			uv.z = uvNode[2].as<float>();
			uv.w = uvNode[3].as<float>();

			m_tileNames.push_back(name);
			m_tileUVMap[name] = uv;
		}

		return true;
	}
}