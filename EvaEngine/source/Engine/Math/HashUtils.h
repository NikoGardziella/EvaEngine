#pragma once
#include <cstdint>
#include <string_view>
#include <cmath>
#include <entt.hpp>
#include <glm/vec2.hpp>

namespace HashUtils
{
    // Simple 64-bit string hash (FNV-1a)
    inline uint64_t HashString64(std::string_view s)
    {
        const uint64_t FNV_OFFSET = 14695981039346656037ull;
        const uint64_t FNV_PRIME = 1099511628211ull;
        uint64_t h = FNV_OFFSET;
        for (char c : s) { h ^= uint8_t(c); h *= FNV_PRIME; }
        return h;
    }

    // Mix helper (splitmix64-style)
    inline uint64_t Mix(uint64_t x)
    {
        x += 0x9e3779b97f4a7c15ull;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ull;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebull;
        return x ^ (x >> 31);
    }
    inline void HashCombine(uint64_t& h, uint64_t v) { h ^= Mix(v + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2)); }

    // Quantize a local position to integer tile coords (avoid float wobble)
    inline glm::ivec2 QuantizeToTile(const glm::vec2& localPos, float tileSizeWorld)
    {
        // round to nearest tile index; use floor if your grid origin expects that
        const float inv = 1.0f / tileSizeWorld;
        int gx = (int)std::lround(localPos.x * inv);
        int gy = (int)std::lround(localPos.y * inv);
        return { gx, gy };
    }

    // IDComponent is assumed to have: struct IDComponent { uint64_t ID; };
    template<typename IDComponent>
    inline uint64_t BaseEntityID(const entt::registry& reg, entt::entity e)
    {
        if (reg.any_of<IDComponent>(e)) return reg.get<IDComponent>(e).ID;
        return (uint64_t)entt::to_integral(e);
    }

    // Main: entity + local tile coords -> 64-bit UID
    inline static  uint64_t MakeTileUID(const uint64_t entID, const glm::vec2& localPos,
        float tileSizeWorld, uint32_t layerOrVariant = 0, uint64_t nameHash = 0)
    {
        uint64_t h = 0xcbf29ce484222325ull; // seed
        const glm::ivec2 t = QuantizeToTile(localPos, tileSizeWorld);

        HashCombine(h, entID);
        HashCombine(h, (uint64_t)(uint32_t)t.x);
        HashCombine(h, (uint64_t)(uint32_t)t.y);
        HashCombine(h, (uint64_t)layerOrVariant);
        HashCombine(h, nameHash);
        return h ? h : 1ull; // avoid 0 if you use 0 as "invalid"
    }

    // Convenience if you just want “entity + index in vector”
    template<typename IDComponent>
    inline uint64_t MakeTileUID_ByIndex(const entt::registry& reg,
        entt::entity e,
        uint32_t index)
    {
        uint64_t h = 0xcbf29ce484222325ull;
        HashCombine(h, BaseEntityID<IDComponent>(reg, e));
        HashCombine(h, (uint64_t)index);
        return h ? h : 1ull;
    }

    inline uint64_t Hash64(std::string_view s) noexcept {
        constexpr uint64_t FNV_OFFSET = 14695981039346656037ull;
        constexpr uint64_t FNV_PRIME = 1099511628211ull;
        uint64_t h = FNV_OFFSET;
        for (unsigned char c : s) { h ^= c; h *= FNV_PRIME; }
        return h;
    }

}
