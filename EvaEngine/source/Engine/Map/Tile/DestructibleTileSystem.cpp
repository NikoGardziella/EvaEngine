#include "pch.h"
#include "DestructibleTileSystem.h"
#include <algorithm>
#include <deque>
#include <Engine/Core/Core.h>
#include "glm/glm.hpp"
#include <Engine/Renderer/Utils/DeltaBitReader.h>
#include <Engine/Map/Grid/TileCollisionMask.h>

static constexpr int TILE_PIX_W = TILE_PIXEL_WIDTH; 
static constexpr int TILE_PIX_H = TILE_PIXEL_HEIGHT;

static inline bool AliveAt(const std::vector<uint8_t>& bits, int W, int x, int y) {
    if ((unsigned)x >= (unsigned)W) return false;
    if ((unsigned)y >= (unsigned)TILE_PIX_H) return false;
    const int idx = y * W + x;
    return (bits[idx >> 3] >> (idx & 7)) & 1;
}

namespace Engine
{
    // LSB-at-x=0 test for a row with explicit stride (bytes per row).
    static inline bool BitAtLSB(const std::vector<uint8_t>& buf, int W, int H, int strideBytes, int x, int y)
    {
        if ((unsigned)x >= (unsigned)W || (unsigned)y >= (unsigned)H) return false;
        const size_t rowBase = (size_t)y * (size_t)strideBytes;
        const int    bitIdx = x;                   // LSB = x=0
        const int    byteIdx = bitIdx >> 3;
        const int    bitOff = bitIdx & 7;
        const uint8_t b = buf[rowBase + byteIdx];
        return (b >> bitOff) & 1u;
    }

    // If strideBytes unknown, infer it safely from buffer size.
    static inline int InferStrideBytesOrMinus1(const std::vector<uint8_t>& bits, int W, int H)
    {
        if (H <= 0) return -1;
        const size_t sz = bits.size();
        if (sz % (size_t)H != 0) return -1;
        return (int)(sz / (size_t)H);
    }

    // Reference: 4-connected top->bottom connectivity for SOLID pixels (alive=1).
    bool DestructibleTileSystem::TopBottomConnectedBits_Ref(const std::vector<uint8_t>& aliveBits, int W, int H)
    {
        if (aliveBits.empty() || W <= 0 || H <= 0) return false;

        // Prefer explicit stride if you know it; otherwise infer:
        int strideBytes = InferStrideBytesOrMinus1(aliveBits, W, H);
        if (strideBytes < 0) {
            EE_CORE_WARN("[DestructibleTileSystem] Bit buffer size {} not divisible by H={}, can't infer stride; W={} (expected ~{} bytes/row).",
                aliveBits.size(), H, W, (W + 7) / 8);
            return false;
        }

        std::deque<std::pair<int, int>> q;
        std::vector<uint8_t> visited((size_t)W * (size_t)H, 0);

        auto pushIf = [&](int x, int y)
            {
                if ((unsigned)x >= (unsigned)W || (unsigned)y >= (unsigned)H) return;
                const int id = y * W + x;
                if (visited[id]) return;

                // SOLID connectivity: require alive==1 here.
                // If you want EMPTY/air connectivity, change to: if (!BitAtLSB(...)) { ... }
                if (!BitAtLSB(aliveBits, W, H, strideBytes, x, y)) return;

                visited[id] = 1;
                q.emplace_back(x, y);
            };

        // Seed: all solid pixels on the top row
        for (int x = 0; x < W; ++x)
            if (BitAtLSB(aliveBits, W, H, strideBytes, x, 0))
                pushIf(x, 0);

        while (!q.empty()) {
            auto [x, y] = q.front(); q.pop_front();
            if (y == H - 1) return true; // reached bottom

            // 4-connected neighbors
            pushIf(x - 1, y);
            pushIf(x + 1, y);
            pushIf(x, y - 1);
            pushIf(x, y + 1);
        }
        return false; // no solid path from top to bottom
    }


    void DestructibleTileSystem::OnTileUpdated()
    {

        //disable
        return;

        EE_PROFILE_FUNCTION();

        auto& Tiles = Engine::TileBlockedMaskCPU::DirtyTileRuntime;
        if (Tiles.empty()) return;

        // Ensure state vectors are large enough
        uint32_t maxSlot = 0;
        for (const auto& tr : Tiles) maxSlot = std::max(maxSlot, tr.slot);
        const size_t need = size_t(maxSlot) + 1;
        if (m_topBottomConnected.size() < need) {
            m_topBottomConnected.resize(need, false);
            m_initialized.resize(need, false);
        }

        // Expect tight packing per tile; warn if not (but still proceed)
        const size_t expected = ((size_t)TILE_PIXEL_WIDTH + 7) / 8 * (size_t)TILE_PIXEL_HEIGHT;

        for (const auto& tr : Tiles)
        {
            const uint32_t slot = tr.slot;

            if (tr.aliveBits.size() != expected) {
                EE_CORE_WARN("[DestructibleTileSystem] slot {}: aliveBits.size()={} != {} (W={},H={})",
                    slot, tr.aliveBits.size(), expected, TILE_PIXEL_WIDTH, TILE_PIXEL_HEIGHT);
                if (tr.aliveBits.empty()) continue; // nothing to do
            }

            // 4-connected, stride-safe check (your ref function)
            const bool now = TopBottomConnectedBits_Ref(tr.aliveBits, TILE_PIXEL_WIDTH, TILE_PIXEL_HEIGHT);

            if (now == false)
            {
                EE_CORE_INFO("now");
            }

            if (!m_initialized[slot]) {
                m_initialized[slot] = true;
                m_topBottomConnected[slot] = now;   // baseline without log
                continue;
            }

            const bool prev = m_topBottomConnected[slot];
            if (prev && !now)
            {
                EE_CORE_INFO("[DestructibleTileSystem] Tile slot {} lost top-to-bottom connectivity", slot);
            }
            else if (!prev && now)
            {
                EE_CORE_INFO("[DestructibleTileSystem] Tile slot {} regained top-to-bottom connectivity", slot);
            }
            m_topBottomConnected[slot] = now;
        }
    }








}