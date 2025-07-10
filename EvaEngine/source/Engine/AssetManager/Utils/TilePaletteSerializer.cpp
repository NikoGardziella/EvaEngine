#include "pch.h"

#include "TilePaletteSerializer.h"
#include <Engine/Core/Log.h>


#include <yaml-cpp/yaml.h>


namespace Engine {


    void TilePaletteSerializer::Serialize(const std::string& filepath,
        const std::vector<std::string>& tileNames,
        const std::unordered_map<std::string, glm::vec4>& tileUVMap)
    {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "TilePalette" << YAML::Value << YAML::BeginSeq;

        for (const auto& name : tileNames)
        {
            const glm::vec4& uv = tileUVMap.at(name);
            out << YAML::BeginMap;
            out << YAML::Key << "Name" << YAML::Value << name;
            out << YAML::Key << "UV" << YAML::Value << YAML::Flow << YAML::BeginSeq << uv.x << uv.y << uv.z << uv.w << YAML::EndSeq;
            out << YAML::EndMap;
        }

        out << YAML::EndSeq;
        out << YAML::EndMap;

        std::ofstream fout(filepath);
        fout << out.c_str();
    }

    bool TilePaletteSerializer::Deserialize(const std::string& filepath,
        std::vector<std::string>& outTileNames,
        std::unordered_map<std::string, glm::vec4>& outTileUVMap)
    {
        std::ifstream stream(filepath);
        if (!stream.is_open())
            return false;

        YAML::Node data = YAML::Load(stream);
        if (!data["TilePalette"])
            return false;

        outTileNames.clear();
        outTileUVMap.clear();

        for (auto entry : data["TilePalette"])
        {
            std::string name = entry["Name"].as<std::string>();
            auto uvNode = entry["UV"];
            glm::vec4 uv = {
                uvNode[0].as<float>(),
                uvNode[1].as<float>(),
                uvNode[2].as<float>(),
                uvNode[3].as<float>()
            };

            outTileNames.push_back(name);
            outTileUVMap[name] = uv;
        }

        return true;
    }

}
