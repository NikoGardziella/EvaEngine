#include "pch.h"
#include "TileStreaming.h"

#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Components/Render/TileComponent.h>
#include <Engine/Renderer/Renderer2D/VulkanRenderer2D.h>

#include <fstream>
#include <filesystem>
#include <algorithm>
#include "Utils/CompressUtils.h"
#include <Engine/Core/Config.h>
#include "TileManager.h"

namespace Engine {


    void TileStreamingSystem::InitTileStreaming(TileManager* tilemanager, const StreamingConfig& config)
    {
        m_tilemanager = tilemanager;
        m_config = config;
        m_initialized = true;
        EnsureCacheDirectory();
        EE_CORE_INFO("TileStreaming: initialized (gpu={}, cpu={}, hysteresis={}, budget={}/frame)",
            m_config.gpuRadius, m_config.cpuRadius, m_config.hysteresis, m_config.maxTransitionsPerFrame);
    }


    void TileStreamingSystem::RegisterTile(uint64_t uid, glm::vec2 worldCenter, const std::vector<uint8_t>& colorData, const std::vector<uint8_t>& propsData, 
        const glm::ivec2& opaqueMin, const glm::ivec2& opaqueMax, const std::string& tileName, uint32_t gpuSlot)
    {
        if (gpuSlot != UINT32_MAX)
        {
            m_slotToUID[gpuSlot] = uid;
        }

        TileStreamEntry entry;
        entry.uid         = uid;
        entry.worldCenter = worldCenter;
        entry.colorData   = colorData;
        entry.propsData   = propsData;
        entry.opaqueMin   = opaqueMin;
        entry.opaqueMax   = opaqueMax;
        entry.tileName    = tileName;
        entry.slot        = gpuSlot;
        entry.residency   = TileResidency::GPU;

        m_entries[uid] = std::move(entry);

    }

    void TileStreamingSystem::UpdateStreaming(Scene* scene, glm::vec2 playerPos)
    {
        EE_PROFILE_FUNCTION();
        if (!m_initialized || m_entries.empty()) return;

        // Process pending evictions from last frame
        auto bindless = VulkanRenderer2D::GetBindlessDescriptorSetRenderer();
        for (const auto& ev : m_pendingEvictions)
        {
            bindless->EvictTile(ev.uid);

        }
        m_pendingEvictions.clear();

        // Build priority list — closest tiles first
        m_priorityList.clear();
        for (auto& [uid, entry] : m_entries)
        {
            float dist = glm::distance(entry.worldCenter, playerPos);
            m_priorityList.push_back({ uid, dist });
        }

        // Partial sort — only need the closest N
        int budget = m_config.maxTransitionsPerFrame;
        int sortCount = std::min(budget * 4, (int)m_priorityList.size());
        std::partial_sort(m_priorityList.begin(),
            m_priorityList.begin() + sortCount,
            m_priorityList.end(),
            [](const auto& a, const auto& b) { return a.dist < b.dist; });

        // Hysteresis thresholds
        float gpuLoadRadius = m_config.gpuRadius;
        float gpuUnloadRadius = m_config.gpuRadius + m_config.hysteresis;
        float cpuLoadRadius = m_config.cpuRadius;
        float cpuUnloadRadius = m_config.cpuRadius + m_config.hysteresis;

        int transitions = 0;
        for (const auto& p : m_priorityList)
        {
            if (transitions >= budget)
            {
                break;

            }

            auto& entry = m_entries[p.uid];
            float dist = p.dist;

            switch (entry.residency)
            {
            case TileResidency::GPU:
                if (dist > gpuUnloadRadius)
                {
                    EvictFromGPU(entry);
                    InvalidateTileSlot(scene, entry.uid);
                    transitions++;

                    if (dist > cpuUnloadRadius)
                    {
                        FlushToDisk(entry);
                        transitions++;
                    }
                }
                break;

            case TileResidency::CPU:
                if (dist <= gpuLoadRadius)
                {
                    UploadToGPU(entry, scene);
                    transitions++;
                }
                else if (dist > cpuUnloadRadius)
                {
                    FlushToDisk(entry);
                    transitions++;
                }
                break;

            case TileResidency::Disk:
                if (dist <= gpuLoadRadius)
                {
                    LoadFromDisk(entry);
                    transitions++;
                    if (transitions < budget)
                    {
                        UploadToGPU(entry, scene);
                        transitions++;
                    }
                }
                else if (dist <= cpuLoadRadius)
                {
                    LoadFromDisk(entry);
                    transitions++;
                }
                break;
            }
        }
    }

    // GPU -> CPU
    void TileStreamingSystem::EvictFromGPU(TileStreamEntry& entry)
    {
        if (entry.residency != TileResidency::GPU) return;

        ReadbackFromGPU(entry);

        m_pendingEvictions.push_back({ entry.uid, entry.slot });

        entry.slot = UINT32_MAX;
        entry.residency = TileResidency::CPU;
    }


    // CPU -> GPU

    void TileStreamingSystem::UploadToGPU(TileStreamEntry& entry, Scene* scene)
    {
        EE_PROFILE_FUNCTION();

        if (entry.residency != TileResidency::CPU)
        {
            return;

        }

        if (entry.colorData.empty() || entry.propsData.empty())
        {
            EE_CORE_WARN("TileStreaming: cannot upload uid {:016x}, no CPU data", entry.uid);
            return;
        }

        VulkanContext* ctx = VulkanContext::Get();
        VkCommandBuffer cb = ctx->BeginSingleTimeCommands();

        auto bindless = VulkanRenderer2D::GetBindlessDescriptorSetRenderer();
        entry.slot = bindless->EnsureTileResidentFromRaw(entry.uid, entry.colorData.data(), entry.colorData.size(),
            entry.propsData.data(), entry.propsData.size(), cb);

        ctx->EndSingleTimeCommands(cb);

        UpdateTileSlot(scene, entry.uid, entry.slot);

        EE_CORE_TRACE("TileStreaming: CPU -> GPU [uid={:016x} '{}' slot={}]", entry.uid, entry.tileName, entry.slot);

        entry.residency = TileResidency::GPU;
    }

    // CPU → Disk
    void TileStreamingSystem::FlushToDisk(TileStreamEntry& entry)
    {
        EE_PROFILE_FUNCTION();

        if (entry.residency != TileResidency::CPU)
        {
            return;
        }

        if (!entry.dirty)
        {
            // Tile was never modified — can be reconstructed from atlas later
            // No need to write to disk at all
            entry.colorData.clear();
            entry.colorData.shrink_to_fit();
            entry.propsData.clear();
            entry.propsData.shrink_to_fit();

            EE_CORE_TRACE("TileStreaming: CPU -> Disk (clean, no write) [uid={:016x} '{}']",
                entry.uid, entry.tileName);

            entry.residency = TileResidency::Disk;
            return;
        }

        if (entry.colorData.empty())
        {
            return;
        }

        std::string path = GetDiskPath(entry.uid);
        std::ofstream out(path, std::ios::binary);
        if (!out.is_open())
        {
            EE_CORE_ERROR("TileStreaming: failed to write '{}'", path);
            return;
        }

        // Sub-rect from opaque bounds
        int x0 = entry.opaqueMin.x;
        int y0 = entry.opaqueMin.y;
        int w = entry.opaqueMax.x - entry.opaqueMin.x;
        int h = entry.opaqueMax.y - entry.opaqueMin.y;

        // Clamp to valid range
        x0 = std::max(0, x0);
        y0 = std::max(0, y0);

        int maxW = TILE_PIXEL_WIDTH - x0;
        w = std::clamp(w, 0, maxW);
        int maxH = TILE_PIXEL_HEIGHT - y0;
        h = std::clamp(h, 0, maxH);

        std::vector<uint8_t> colorSub = CompressUtils::ExtractSubRect(entry.colorData, TILE_PIXEL_WIDTH, TILE_PIXEL_HEIGHT, x0, y0, w, h);
        std::vector<uint8_t> propsSub = CompressUtils::ExtractSubRect(entry.propsData, TILE_PIXEL_WIDTH, TILE_PIXEL_HEIGHT, x0, y0, w, h);

        std::vector<uint8_t> colorCompressed = CompressUtils::CompressLZ4HC(colorSub.data(), static_cast<uint32_t>(colorSub.size()));
        std::vector<uint8_t> propsCompressed = CompressUtils::CompressLZ4HC(propsSub.data(), static_cast<uint32_t>(propsSub.size()));

        if (colorCompressed.empty() || propsCompressed.empty())
        {
            EE_CORE_ERROR("TileStreaming: compression failed for uid {:016x}", entry.uid);
            out.close();
            return;
        }

        // Write header
        uint32_t magic = TILE_FILE_MAGIC;
        uint32_t version = TILE_FILE_VERSION;
        out.write((const char*)&magic, 4);
        out.write((const char*)&version, 4);

        // Write sub-rect bounds
        out.write((const char*)&x0, 4);
        out.write((const char*)&y0, 4);
        out.write((const char*)&w, 4);
        out.write((const char*)&h, 4);

        // Write opaque bounds
        out.write((const char*)&entry.opaqueMin, sizeof(glm::ivec2));
        out.write((const char*)&entry.opaqueMax, sizeof(glm::ivec2));

        // Write color: original size, compressed size, data
        uint32_t colorOrigSize = static_cast<uint32_t>(colorSub.size());
        uint32_t colorCompSize = static_cast<uint32_t>(colorCompressed.size());
        out.write((const char*)&colorOrigSize, 4);
        out.write((const char*)&colorCompSize, 4);
        out.write((const char*)colorCompressed.data(), colorCompSize);

        // Write props: original size, compressed size, data
        uint32_t propsOrigSize = static_cast<uint32_t>(propsSub.size());
        uint32_t propsCompSize = static_cast<uint32_t>(propsCompressed.size());
        out.write((const char*)&propsOrigSize, 4);
        out.write((const char*)&propsCompSize, 4);
        out.write((const char*)propsCompressed.data(), propsCompSize);

        out.close();

        // Stats
        uint32_t fullSize = static_cast<uint32_t>(entry.colorData.size() + entry.propsData.size());
        uint32_t savedSize = colorCompSize + propsCompSize;
        float ratio = fullSize > 0 ? (float(savedSize) / float(fullSize)) * 100.0f : 0.0f;

        EE_CORE_TRACE("TileStreaming: CPU -> Disk [uid={:016x} '{}' rect={}x{} full={} saved={} ({:.1f}%)]",
            entry.uid, entry.tileName, w, h, fullSize, savedSize, ratio);

        // Free RAM
        entry.colorData.clear();
        entry.colorData.shrink_to_fit();
        entry.propsData.clear();
        entry.propsData.shrink_to_fit();

        entry.residency = TileResidency::Disk;
    }


    // Disk -> CPU
    void TileStreamingSystem::LoadFromDisk(TileStreamEntry& entry)
    {
        EE_PROFILE_FUNCTION();

        if (entry.residency != TileResidency::Disk)
        {
            return;

        }

        if (!entry.dirty)
        {
            // Clean tile — reconstruct from atlas, no disk read
            ReconstructFromAtlas(entry);
            entry.residency = TileResidency::CPU;
            EE_CORE_TRACE("TileStreaming: Disk -> CPU (from atlas) [uid={:016x} '{}']",
                entry.uid, entry.tileName);
            return;
        }

        // Dirty tile — read from disk
        std::string path = GetDiskPath(entry.uid);
        std::ifstream in(path, std::ios::binary);
        if (!in.is_open())
        {
            EE_CORE_ERROR("TileStreaming: failed to read '{}', falling back to atlas", path);
            ReconstructFromAtlas(entry);
            entry.dirty = false;
            entry.residency = TileResidency::CPU;
            return;
        }

        // Read and verify header
        uint32_t magic, version;
        in.read((char*)&magic, 4);
        in.read((char*)&version, 4);

        if (magic != TILE_FILE_MAGIC || version != TILE_FILE_VERSION)
        {
            EE_CORE_ERROR("TileStreaming: invalid file format '{}', falling back to atlas", path);
            in.close();
            ReconstructFromAtlas(entry);
            entry.dirty = false;
            entry.residency = TileResidency::CPU;
            return;
        }

        // Read sub-rect bounds
        int x0, y0, w, h;
        in.read((char*)&x0, 4);
        in.read((char*)&y0, 4);
        in.read((char*)&w, 4);
        in.read((char*)&h, 4);

        // Read opaque bounds
        in.read((char*)&entry.opaqueMin, sizeof(glm::ivec2));
        in.read((char*)&entry.opaqueMax, sizeof(glm::ivec2));

        // Read color
        uint32_t colorOrigSize, colorCompSize;
        in.read((char*)&colorOrigSize, 4);
        in.read((char*)&colorCompSize, 4);
        std::vector<uint8_t> colorCompressed(colorCompSize);
        in.read((char*)colorCompressed.data(), colorCompSize);

        std::vector<uint8_t> colorSub;
        if (!CompressUtils::DecompressLZ4(colorCompressed, colorSub, colorOrigSize))
        {
            in.close();
            ReconstructFromAtlas(entry);
            entry.dirty = false;
            entry.residency = TileResidency::CPU;
            return;
        }

        // Read props
        uint32_t propsOrigSize, propsCompSize;
        in.read((char*)&propsOrigSize, 4);
        in.read((char*)&propsCompSize, 4);
        std::vector<uint8_t> propsCompressed(propsCompSize);
        in.read((char*)propsCompressed.data(), propsCompSize);

        std::vector<uint8_t> propsSub;
        if (!CompressUtils::DecompressLZ4(propsCompressed, propsSub, propsOrigSize))
        {
            in.close();
            ReconstructFromAtlas(entry);
            entry.dirty = false;
            entry.residency = TileResidency::CPU;
            return;
        }

        in.close();

        // Reconstruct full-size buffers from sub-rect
        CompressUtils::InsertSubRect(entry.colorData, colorSub, TILE_PIXEL_WIDTH, TILE_PIXEL_HEIGHT, x0, y0, w, h);
        CompressUtils::InsertSubRect(entry.propsData, propsSub, TILE_PIXEL_WIDTH, TILE_PIXEL_HEIGHT, x0, y0, w, h);

        EE_CORE_TRACE("TileStreaming: Disk -> CPU [uid={:016x} '{}' rect={}x{}]",
            entry.uid, entry.tileName, w, h);

        entry.residency = TileResidency::CPU;
    }

    void TileStreamingSystem::ReconstructFromAtlas(TileStreamEntry& entry)
    {
        EE_PROFILE_FUNCTION();

        if (!m_tilemanager)
        {
            EE_CORE_ERROR("TileStreaming: no TileManager for reconstruction");
            entry.colorData.assign(size_t(TILE_PIXEL_WIDTH) * TILE_PIXEL_HEIGHT * 4, 0);
            entry.propsData.assign(size_t(TILE_PIXEL_WIDTH) * TILE_PIXEL_HEIGHT * 4, 0);
            return;
        }

        const std::vector<uint8_t>* color = nullptr;
        const std::vector<uint8_t>* props = nullptr;

        if (m_tilemanager->GetOriginalTileData(entry.uid, color, props))
        {
            entry.colorData = *color;
            entry.propsData = *props;
            EE_CORE_TRACE("TileStreaming: reconstructed from atlas [uid={:016x}]", entry.uid);
        }
        else
        {
            EE_CORE_ERROR("TileStreaming: no atlas data for uid {:016x}", entry.uid);
            entry.colorData.assign(size_t(TILE_PIXEL_WIDTH) * TILE_PIXEL_HEIGHT * 4, 0);
            entry.propsData.assign(size_t(TILE_PIXEL_WIDTH) * TILE_PIXEL_HEIGHT * 4, 0);
        }
    }

    // Update TileComponent slots after GPU upload

        void TileStreamingSystem::UpdateTileSlot(Scene* scene, uint64_t uid, uint32_t newSlot)
    {
        scene->ForEach<TileComponent>([&](Entity e, TileComponent& tc)
        {
            for (auto& t : tc.tiles)
            {
                if (t.UID == uid)
                {
                    t.Slot = newSlot;
                }
            }
        });
    }

    // Invalidate slot when evicted from GPU
    void TileStreamingSystem::InvalidateTileSlot(Scene* scene, uint64_t uid)
    {
        scene->ForEach<TileComponent>([&](Entity e, TileComponent& tc)
        {
            for (auto& t : tc.tiles)
            {
                if (t.UID == uid)
                {
                    t.Slot = UINT32_MAX;
                }
            }
        });
    }

    // Queries
    TileResidency TileStreamingSystem::GetResidency(uint64_t uid) const
    {
        auto it = m_entries.find(uid);
        if (it == m_entries.end()) return TileResidency::Disk;
        return it->second.residency;
    }

    uint32_t TileStreamingSystem::GetSlot(uint64_t uid) const
    {
        auto it = m_entries.find(uid);
        if (it == m_entries.end()) return UINT32_MAX;
        return it->second.slot;
    }

    uint32_t TileStreamingSystem::GetGPUResidentCount() const
    {
        uint32_t count = 0;
        for (const auto& [uid, e] : m_entries)
            if (e.residency == TileResidency::GPU) count++;
        return count;
    }

    uint32_t TileStreamingSystem::GetCPUResidentCount() const
    {
        uint32_t count = 0;
        for (const auto& [uid, e] : m_entries)
            if (e.residency == TileResidency::CPU) count++;
        return count;
    }

    uint32_t TileStreamingSystem::GetDiskResidentCount() const
    {
        uint32_t count = 0;
        for (const auto& [uid, e] : m_entries)
            if (e.residency == TileResidency::Disk) count++;
        return count;
    }

    void TileStreamingSystem::ReadbackFromGPU(TileStreamEntry& entry)
    {
        if (entry.residency != TileResidency::GPU || entry.slot == UINT32_MAX) return;

        auto bindless = VulkanRenderer2D::GetBindlessDescriptorSetRenderer();

        // Read color layer back from GPU
        bindless->ReadbackArrayLayer(entry.slot, entry.colorData, entry.propsData);
    }

    void TileStreamingSystem::SyncDirtyFlags(const std::vector<DirtyTileRuntime>& dirtyTiles)
    {
        for (const auto& dt : dirtyTiles)
        {
            auto slotIt = m_slotToUID.find(dt.slot);
            if (slotIt == m_slotToUID.end()) continue;

            auto entryIt = m_entries.find(slotIt->second);
            if (entryIt == m_entries.end()) continue;

            entryIt->second.dirty = true;
        }

    }

    // Force all to GPU
    void TileStreamingSystem::ForceAllToGPU(Scene* scene)
    {
        for (auto& [uid, entry] : m_entries)
        {
            if (entry.residency == TileResidency::Disk)
                LoadFromDisk(entry);
            if (entry.residency == TileResidency::CPU)
                UploadToGPU(entry, scene);
        }
    }

    // Flush all to disk
    void TileStreamingSystem::FlushAllToDisk()
    {
        for (auto& [uid, entry] : m_entries)
        {
            if (entry.residency == TileResidency::GPU)
                EvictFromGPU(entry);
            if (entry.residency == TileResidency::CPU)
                FlushToDisk(entry);
        }
    }

    // Shutdown
    void TileStreamingSystem::Shutdown()
    {
        FlushAllToDisk();
        m_entries.clear();
        m_initialized = false;
        EE_CORE_INFO("TileStreaming: shutdown");
    }

    // Disk path helper
    std::string TileStreamingSystem::GetDiskPath(uint64_t uid) const
    {
        return m_config.cachePath + "/" + std::to_string(uid) + ".tile";
    }

    void TileStreamingSystem::EnsureCacheDirectory() const
    {
        std::filesystem::create_directories(m_config.cachePath);
    }




}
