#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include "AnimationTypes.h"

namespace Engine {

    class TextureStreamingSystem; // fwd

    // Owns loaded clips and connects to your streaming system
    class AnimationBank {
    public:
        explicit AnimationBank(){}

        uint32_t LoadClipFromYaml(const std::string& yamlPath);

        // Public API
        uint32_t LoadGridClip(const std::string& name,       // "player/run"
            const std::string& texturePath,// e.g. "animations/player/spritesheet/run.png"
            uint16_t cols, uint16_t rows,
            uint16_t cellW, uint16_t cellH,
            float fps, bool loop,
            glm::u16vec2 pivotPx,
            float pixelsPerUnit);

        const AnimationClipRuntime* GetClip(uint32_t clipId) const;
        void ReleaseUnusedLRU(uint32_t framesSinceUseThreshold);

        // Optional: reference counting
        void AddUser(uint32_t clipId);
        void RemoveUser(uint32_t clipId);

    private:
        struct ClipSlot {
            AnimationClipRuntime clip;
            uint32_t users = 0;
            uint32_t lastUsedFrame = 0;
            bool     loaded = false;
        };

        std::unordered_map<uint32_t, ClipSlot> m_clips; // key = Hash32(name)

        // helpers
        static uint32_t HashName(const std::string& s);
        void BuildUVTable(ClipSlot& slot);
    };

}
