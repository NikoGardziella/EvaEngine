#pragma once
#include "json.hpp"
#include <glm/ext/vector_float2.hpp>
#include <Engine/Scene/Entity.h>


class ParseUtils
{
public:

	static bool ParseComponent(std::string compName, Engine::Entity entity, const nlohmann::json& compData);
	
private:
	template<typename T>
	static bool SafeGet(const nlohmann::json& j, const std::string& key, T& out);

	static bool SafeGetVec2(const nlohmann::json& j, const std::string& key, glm::vec2& out);

	static bool SafeGetVec3(const nlohmann::json& j, const std::string& key, glm::vec3& out);


};

