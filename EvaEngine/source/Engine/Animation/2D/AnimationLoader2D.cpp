#include "pch.h"
#include "AnimationLoader2D.h"
#include "AnimationBank2D.h"
#include <Engine/Core/Log.h>
#include <yaml-cpp/yaml.h>
#include <stdexcept>
#include <string>

namespace Engine {

    static inline uint16_t ReadU16(const YAML::Node& n, const char* key, uint16_t def)
    {
        auto k = n[key];
        return k ? static_cast<uint16_t>(k.as<int>()) : def;
    }
    static inline float ReadF32(const YAML::Node& n, const char* key, float def)
    {
        auto k = n[key];
        return k ? k.as<float>() : def;
    }
    static inline bool ReadBool(const YAML::Node& n, const char* key, bool def)
    {
        auto k = n[key];
        return k ? k.as<bool>() : def;
    }

    uint32_t AnimationLoader2D::Load2DGridYaml(AnimationBank2D& bank, const std::string& yamlPath)
    {
        YAML::Node root;
        try {
            root = YAML::LoadFile(yamlPath);
        }
        catch (const std::exception& e)
        {
            EE_CORE_ERROR("[AnimationLoader] YAML parse failed '{}': {}", yamlPath, e.what());
            return 0;
        }

        // Required
        const std::string pack = root["pack"] ? root["pack"].as<std::string>() : "";
        const std::string action = root["action"] ? root["action"].as<std::string>() : "";
        const std::string texture = root["texture"] ? root["texture"].as<std::string>() : "";

        if (pack.empty() || action.empty() || texture.empty()) 
        {
            EE_CORE_ERROR("[AnimationLoader] Missing required fields in '{}'. Need 'pack', 'action', 'texture'", yamlPath);
            return 0;
        }

        const auto frameSize = root["frameSizePx"];
        if (!frameSize || !frameSize["w"] || !frameSize["h"])
        {
            EE_CORE_ERROR("[AnimationLoader] Missing 'frameSizePx' {w,h} in '{}'", yamlPath);
            return 0;
        }
        const uint16_t cellW = static_cast<uint16_t>(frameSize["w"].as<int>());
        const uint16_t cellH = static_cast<uint16_t>(frameSize["h"].as<int>());

        const auto grid = root["grid"];
        if (!grid || !grid["cols"] || !grid["rows"])
        {
            EE_CORE_ERROR("[AnimationLoader] Missing 'grid' {cols,rows} in '{}'", yamlPath);
            return 0;
        }
        const uint16_t cols = static_cast<uint16_t>(grid["cols"].as<int>());
        const uint16_t rows = static_cast<uint16_t>(grid["rows"].as<int>());

        const float fps = ReadF32(root, "fps", 12.0f);
        const bool loop = ReadBool(root, "loop", true);

        glm::u16vec2 pivotPx{ 0,0 };
        if (auto p = root["pivotPx"]; p && p["x"] && p["y"]) {
            pivotPx.x = static_cast<uint16_t>(p["x"].as<int>());
            pivotPx.y = static_cast<uint16_t>(p["y"].as<int>());
        }
        else 
        {
            // bottom-center default
            pivotPx = { static_cast<uint16_t>(cellW / 2), static_cast<uint16_t>(cellH ? (cellH - 1) : 0) };
        }

        const float ppu = ReadF32(root, "pixelsPerUnit", 64.0f);

        // Compose unique name "pack/action"
        const std::string name = pack + "/" + action;

        // Direction order check (optional)
        if (auto ord = grid["order"])
        {
            // You can validate 8 entries: E, SE, S, SW, W, NW, N, NE
            if (ord.IsSequence() && ord.size() == 8) {
                // If you later store order inside the clip, read it here.
            }
        }

        uint32_t id = bank.Load2DGridClip(name, texture, cols, rows, cellW, cellH, fps, loop, pivotPx, ppu);
        if (id == 0) 
        {
            EE_CORE_ERROR("[AnimationLoader] Failed to load clip '{}' from '{}'", name, yamlPath);
        }
        else 
        {
            EE_CORE_INFO("[AnimationLoader] Loaded '{}' from '{}'", name, yamlPath);
        }
        return id;
    }

} 
