
#include "pch.h"
#include "AnimationBank2D.h"
#include <Engine/Core/Log.h>
#include <Engine/Math/HashUtils.h> 
#include "Engine/AssetManager/AssetManager.h"
#include "Engine/Renderer/VulkanRenderer2D.h"

#include <algorithm>
#include <cmath>
#include <yaml-cpp/yaml.h>
#include <fstream>

namespace Engine {

    // If you already have HashUtils::Hash32, use it. Otherwise fallback to FNV-1a.
    static inline uint32_t Hash32Fallback(const std::string& s) {

        return HashUtils::Hash32(s.c_str());

    }

    uint32_t AnimationBank2D::HashName(const std::string& s)
    {
        return HashUtils::Hash32(s.c_str());
    }

    static inline AnimationFrameUV QuantizeUV(float u0, float v0, float u1, float v1)
    {
        AnimationFrameUV uv{};
        uv.uvMin16.x = static_cast<uint16_t>(std::round(std::clamp(u0, 0.0f, 1.0f) * 65535.0f));
        uv.uvMin16.y = static_cast<uint16_t>(std::round(std::clamp(v0, 0.0f, 1.0f) * 65535.0f));
        uv.uvMax16.x = static_cast<uint16_t>(std::round(std::clamp(u1, 0.0f, 1.0f) * 65535.0f));
        uv.uvMax16.y = static_cast<uint16_t>(std::round(std::clamp(v1, 0.0f, 1.0f) * 65535.0f));
        return uv;
    }

    void AnimationBank2D::BuildUVTable(Clip2DSlot& slot)
    {
        const auto& g = slot.clip.grid;
        const float texW = float(slot.clip.texWidth);
        const float texH = float(slot.clip.texHeight);

        slot.clip.uvTable.resize(g.rows * g.cols);

        // tiny guard against bleeding; adjust if you have gutters
        const float eps = 0.0f; // e.g. set to (0.5f / texW/texH) if needed

        for (uint32_t row = 0; row < g.rows; ++row)
        {
            for (uint32_t col = 0; col < g.cols; ++col)
            {
                const float x0 = float(col * g.cellW);
                const float y0 = float(row * g.cellH);
                const float x1 = x0 + float(g.cellW);
                const float y1 = y0 + float(g.cellH);

                // U as usual
                float u0 = (x0 + eps) / texW;
                float u1 = (x1 - eps) / texW;

                // V source (top->bottom in image space)
                float v0_src = (y0 + eps) / texH;
                float v1_src = (y1 - eps) / texH;

                // Flip V to match your runtime/shader convention
                float v0 = 1.0f - v1_src;
                float v1 = 1.0f - v0_src;

                // Quantize to 16-bit and store
                slot.clip.uvTable[row * g.cols + col] = QuantizeUV(u0, v0, u1, v1);
            }
        }
    }



    static inline bool TryParseDir(const std::string& s, uint8_t& out) {
        static const std::unordered_map<std::string, uint8_t> map = {
            {"E",0},{"SE",1},{"S",2},{"SW",3},{"W",4},{"NW",5},{"N",6},{"NE",7}
        };
        auto it = map.find(s);
        if (it == map.end()) return false;
        out = it->second;
        return true;
    }

    uint32_t AnimationBank2D::Load2DClipFromYaml(const std::string& yamlPath)
    {
        // 1) Resolve YAML to an absolute path inside your asset root
        std::filesystem::path assetRoot = AssetManager::GetAssetFolderPath();
        std::filesystem::path absYaml = (assetRoot / std::filesystem::path(yamlPath)).lexically_normal();

        // If yamlPath is already absolute, prefer it
        if (std::filesystem::path(yamlPath).is_absolute())
            absYaml = std::filesystem::path(yamlPath);

        // If the combined path doesn't exist but yamlPath exists as given, use that
        if (!std::filesystem::exists(absYaml) && std::filesystem::exists(yamlPath))
            absYaml = std::filesystem::path(yamlPath);

        if (!std::filesystem::exists(absYaml)) {
            EE_CORE_ERROR("[AnimationBank] YAML not found. Given='{}'  AssetRoot='{}'  Resolved='{}'  CWD='{}'",
                yamlPath, assetRoot.string(), absYaml.string(), std::filesystem::current_path().string());

            // Helpful directory listings
            if (std::filesystem::exists(assetRoot)) {
                std::string entries;
                for (auto& e : std::filesystem::directory_iterator(assetRoot))
                    entries += (e.path().filename().string() + " ");
                EE_CORE_ERROR("[AnimationBank] Asset root listing '{}': {}", assetRoot.string(), entries);
            }
            auto parent = absYaml.parent_path();
            if (!parent.empty() && std::filesystem::exists(parent)) {
                std::string entries;
                for (auto& e : std::filesystem::directory_iterator(parent))
                    entries += (e.path().filename().string() + " ");
                EE_CORE_ERROR("[AnimationBank] Dir listing for resolved parent '{}': {}", parent.string(), entries);
            }
            return 0;
        }

        // 2) Read file text (clearer errors than YAML::LoadFile)
        std::ifstream in(absYaml, std::ios::binary);
        if (!in) {
            EE_CORE_ERROR("[AnimationBank] Failed to open YAML '{}'", absYaml.string());
            return 0;
        }
        std::string yamlText((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

        // 3) Parse YAML
        YAML::Node root;
        try {
            root = YAML::Load(yamlText);
        }
        catch (const std::exception& e) {
            EE_CORE_ERROR("[AnimationBank] YAML parse error '{}': {}", absYaml.string(), e.what());
            return 0;
        }



        // 4) Extract fields with guards
        const std::string pack = root["pack"] ? root["pack"].as<std::string>() : "";
        const std::string action = root["action"] ? root["action"].as<std::string>() : "";
        std::string textureName = root["texture"] ? root["texture"].as<std::string>() : "";

        uint16_t cellW = 128, cellH = 128;
        if (auto fs = root["frameSizePx"]) {
            if (fs["w"]) cellW = static_cast<uint16_t>(fs["w"].as<int>());
            if (fs["h"]) cellH = static_cast<uint16_t>(fs["h"].as<int>());
        }

        uint16_t cols = 0, rows = 0;
        YAML::Node grid = root["grid"];
        if (grid && grid.IsMap()) {
            if (grid["cols"]) cols = static_cast<uint16_t>(grid["cols"].as<int>());
            if (grid["rows"]) rows = static_cast<uint16_t>(grid["rows"].as<int>());
        }

        float fps = root["fps"] ? root["fps"].as<float>() : 12.f;
        bool  loop = root["loop"] ? root["loop"].as<bool>() : true;

        glm::uvec2 pivotPx{ cellW / 2u, cellH - 1u };
        if (auto pv = root["pivotPx"]) {
            if (pv["x"]) pivotPx.x = static_cast<uint32_t>(pv["x"].as<int>());
            if (pv["y"]) pivotPx.y = static_cast<uint32_t>(pv["y"].as<int>());
        }

        float ppu = root["pixelsPerUnit"] ? root["pixelsPerUnit"].as<float>() : 64.f;

    
        // 6) Clip id and dedup
        const std::string name = pack.empty() ? action : (pack + "/" + action);
        const uint32_t id = HashName(name.empty() ? yamlPath : name);
        if (auto it = m_2DClips.find(id); it != m_2DClips.end()) {
            it->second.users++;
            return id;
        }

        // 7) Acquire spritesheet (binding=3)
        uint32_t texIndex = 0xFFFFFFFFu, texW = 0, texH = 0;
        bool ok = VulkanRenderer2D::GetBindlessDescriptorSetRenderer()
            ->AcquireSpritesheet(textureName, texIndex, texW, texH);
        if (!ok || texIndex == 0xFFFFFFFFu) {
            EE_CORE_ERROR("[AnimationBank] Failed to acquire spritesheet '{}'", textureName);
            return 0;
        }

        // 8) Validate grid fits the texture
        if (uint32_t(cols) * cellW > texW || uint32_t(rows) * cellH > texH) {
            EE_CORE_ERROR("[AnimationBank] Grid exceeds texture: tex {}x{}, grid {}x{} of {}x{}",
                texW, texH, cols, rows, cellW, cellH);
            VulkanRenderer2D::GetBindlessDescriptorSetRenderer()->ReleaseSpritesheet(texIndex);
            return 0;
        }

        // 9) Build clip
        Clip2DSlot slot{};
        slot.loaded = true; slot.users = 1; slot.lastUsedFrame = 0;
        slot.clip.name = name.empty() ? yamlPath : name;
        slot.clip.grid = { cols, rows, cellW, cellH, fps, static_cast<uint8_t>(loop ? 1 : 0), texIndex };
        slot.clip.frameSizePx = { cellW, cellH };
        slot.clip.pivotPx = { static_cast<uint16_t>(pivotPx.x), static_cast<uint16_t>(pivotPx.y) };
        slot.clip.pixelsPerUnit = ppu;
        slot.clip.texWidth = texW;
        slot.clip.texHeight = texH;

        // 10) dirToRow mapping (default identity)
        for (uint8_t i = 0; i < 8; ++i) slot.clip.dirToRow[i] = i;
        if (grid && grid["order"] && grid["order"].IsSequence()) {
            uint8_t row = 0;
            for (auto d : grid["order"]) {
                const std::string s = d.as<std::string>();
                uint8_t dirIdx = 0;
                if (!TryParseDir(s, dirIdx)) {
                    EE_CORE_WARN("[AnimationBank] Unknown dir '{}' in '{}', using default order", s, yamlPath);
                    for (uint8_t i = 0; i < 8; ++i) slot.clip.dirToRow[i] = i;
                    break;
                }
                if (row < rows) slot.clip.dirToRow[dirIdx] = row;
                ++row;
                if (row >= rows) break;
            }
            // Clamp any leftover entries
            for (uint8_t i = 0; i < 8; ++i)
                if (slot.clip.dirToRow[i] >= rows)
                    slot.clip.dirToRow[i] = std::min<uint8_t>(i, static_cast<uint8_t>(rows - 1));
        }

        // 11) UV table (rows * cols), row-major
        slot.clip.uvTable.resize(static_cast<size_t>(cols) * rows);
        for (uint16_t r = 0; r < rows; ++r)
            for (uint16_t c = 0; c < cols; ++c) {
                const float u0 = float(c * cellW) / float(texW);
                const float v0 = float(r * cellH) / float(texH);
                const float u1 = float((c + 1) * cellW) / float(texW);
                const float v1 = float((r + 1) * cellH) / float(texH);
                const size_t idx = static_cast<size_t>(r) * cols + c;
                slot.clip.uvTable[idx].uvMin16 = glm::uvec2(uint32_t(u0 * 65535.f), uint32_t(v0 * 65535.f));
                slot.clip.uvTable[idx].uvMax16 = glm::uvec2(uint32_t(u1 * 65535.f), uint32_t(v1 * 65535.f));
            }

        // 12) Optional events
        if (auto ev = root["events"]; ev && ev.IsSequence()) {
            for (auto e : ev) {
                if (!e["frame"] || !e["name"]) continue;
                uint32_t f = e["frame"].as<uint32_t>();
                std::string n = e["name"].as<std::string>();
                slot.clip.events[f].push_back(n);
            }
        }

        // 13) Commit
        m_2DClips.emplace(id, std::move(slot));
        EE_CORE_INFO("[AnimationBank] Loaded '{}' from '{}' ({}x{}, grid {}x{}, fps {}, loop {}, texIdx {})",
            name.empty() ? yamlPath : name, absYaml.string(),
            texW, texH, cols, rows, fps, loop, texIndex);
        return id;
    }


    uint32_t AnimationBank2D::Load2DGridClip(const std::string& name,
        const std::string& texturePath,
        uint16_t cols, uint16_t rows,
        uint16_t cellW, uint16_t cellH,
        float fps, bool loop,
        glm::u16vec2 pivotPx,
        float pixelsPerUnit)
    {
        const uint32_t id = HashName(name);
        if (auto it = m_2DClips.find(id); it != m_2DClips.end()) {
            it->second.users++;
            return id;
        }

        // Acquire spritesheet from the bindless renderer (binding=3)
        uint32_t texIndex = 0xFFFFFFFFu;
        uint32_t texW = 0, texH = 0;
        bool ok = VulkanRenderer2D::GetBindlessDescriptorSetRenderer()
            ->AcquireSpritesheet(texturePath, texIndex, texW, texH);

        if (!ok || texIndex == 0xFFFFFFFFu || texW == 0 || texH == 0) {
            EE_CORE_ERROR("[AnimationBank] Failed to acquire spritesheet '{}' for clip '{}'", texturePath, name);
            return 0;
        }

        Clip2DSlot slot{};
        slot.loaded = true;
        slot.users = 1;
        slot.lastUsedFrame = 0;

        slot.clip.name = name;
        slot.clip.grid.cols = cols;
        slot.clip.grid.rows = rows;
        slot.clip.grid.cellW = cellW;
        slot.clip.grid.cellH = cellH;
        slot.clip.grid.fps = fps;
        slot.clip.grid.loop = loop ? 1u : 0u;
        slot.clip.grid.textureIndex = texIndex;

        slot.clip.frameSizePx = { cellW, cellH };
        slot.clip.pivotPx = pivotPx;
        slot.clip.pixelsPerUnit = pixelsPerUnit;
        slot.clip.texWidth = texW;
        slot.clip.texHeight = texH;

        // Sanity: grid fits the texture
        if (uint32_t(cols) * uint32_t(cellW) > texW ||
            uint32_t(rows) * uint32_t(cellH) > texH) {
            EE_CORE_ERROR("[AnimationBank] Grid exceeds texture bounds for clip '{}': tex {}x{}, grid {}x{} of {}x{}",
                name, texW, texH, cols, rows, cellW, cellH);
            // Release the slot we just acquired to avoid leaking it
            VulkanRenderer2D::GetBindlessDescriptorSetRenderer()->ReleaseSpritesheet(texIndex);
            return 0;
        }

        BuildUVTable(slot);

        m_2DClips.emplace(id, std::move(slot));
        EE_CORE_INFO("[AnimationBank] Loaded clip '{}' ({}x{}, grid {}x{}, fps {}, loop {}, texIdx {})",
            name, texW, texH, cols, rows, fps, loop, texIndex);
        return id;
    }


    const Animation2DClipRuntime* AnimationBank2D::Get2DClip(uint32_t clipId) const 
    {
        auto it = m_2DClips.find(clipId);
        if (it == m_2DClips.end()) return nullptr;
        return &it->second.clip;
    }

    void AnimationBank2D::AddUser(uint32_t clipId)
    {
        auto it = m_2DClips.find(clipId);
        if (it != m_2DClips.end()) it->second.users++;
    }

    void AnimationBank2D::RemoveUser(uint32_t clipId)
    {
        auto it = m_2DClips.find(clipId);
        if (it == m_2DClips.end()) return;
        auto& slot = it->second;
        if (slot.users > 0) slot.users--;
        if (slot.users == 0)
        {
            // Free binding=3 slot
            VulkanRenderer2D::GetBindlessDescriptorSetRenderer()
                ->ReleaseSpritesheet(slot.clip.grid.textureIndex);
            m_2DClips.erase(it);
        }
    }

    void AnimationBank2D::ReleaseUnusedLRU(uint32_t framesSinceUseThreshold)
    {
   
    }

}
