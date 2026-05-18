#include "pch.h"
#include "DestrutibleTileSystem.h"
#include <Engine/Map/Grid/TileCollisionMask.h>
#include <Engine/Renderer/Renderer2D/VulkanRenderer2D.h>
#include <Engine/Math/HashUtils.h>
#include <Engine/Scene/Components/Physics/PhysicsComponent.h>

#include <queue>
#include <cstdint>
#include <algorithm>
#include <cmath>

#include <Engine/Scene/Components/Render/TileComponent.h>
#include "Engine/Scene/Components/Physics/PhysicUtils.h"
#include "Engine/Map/Utils/IsoTileUtils.h"
#include "Engine/AssetManager/AssetManager.h"

namespace Engine {


    namespace {
        // Small inline bit testers for flood fill
        inline bool testBitLSB(const uint32_t* words, int W, int x, int y) {
            const int wordsPerRow = (W + 31) / 32;
            const int idx = y * wordsPerRow + (x >> 5);
            const uint32_t bit = 1u << (x & 31);
            return (words[idx] & bit) != 0;
        }
        inline bool testBitMSB(const uint32_t* words, int W, int x, int y) {
            const int wordsPerRow = (W + 31) / 32;
            const int idx = y * wordsPerRow + (x >> 5);
            const int off = 31 - (x & 31);
            const uint32_t bit = 1u << off;
            return (words[idx] & bit) != 0;
        }
    } 
 
    // -------------------------
    // Tunables
    // -------------------------
    int DestructibleTileSystem::MIN_ROW_POP(int W) {
        if (W <= 8) return 1;
        int t = (int)std::ceil(W * 0.02f);    // ~2%
        t = std::max(t, 1);
        t = std::min(t, std::max(2, W / 16)); // cap ~6.25%
        return t;
    }
    int  DestructibleTileSystem::GAP_ROW_POP_THRESHOLD() { return 0; } // "dead row"
    int  DestructibleTileSystem::GAP_MIN_CONSEC_ROWS() { return 2; }
    bool DestructibleTileSystem::USE_8_CONNECTED() { return false; }

    // -------------------------
    // Bit access + CRC
    // -------------------------
    bool DestructibleTileSystem::ReadBitPackedAt(const std::vector<uint32_t>& words,
        int W, int H, int x, int y,  const ConnCfg& cfg)
    {
        if ((unsigned)x >= (unsigned)W || (unsigned)y >= (unsigned)H) return false;
        const size_t idx = size_t(y) * size_t(W) + size_t(x);
        const size_t word = idx >> 5;
        if (word >= words.size()) return false;
        unsigned bit = unsigned(idx & 31u);
        if (!cfg.lsbFirst) bit = 31u - bit;
        const uint32_t v = (words[word] >> bit) & 1u;
        return cfg.oneIsAlive ? (v != 0u) : (v == 0u);
    }

    int DestructibleTileSystem::CountAliveWithSense(const std::vector<uint32_t>& words,
        int W, int H, bool lsbFirst, bool oneIsAlive)
    {
        ConnCfg tmp{ lsbFirst, oneIsAlive, true, -1 };
        int c = 0;
        for (int y = 0; y < H; ++y)
        {
            for (int x = 0; x < W; ++x)
            {
                c += ReadBitPackedAt(words, W, H, x, y, tmp) ? 1 : 0;
            }

        }
        return c;
    }

    uint32_t DestructibleTileSystem::Crc32Words(const std::vector<uint32_t>& v)
    {
        uint32_t h = 0x811C9DC5u; // FNV-ish
        for (uint32_t w : v) { h ^= w; h *= 16777619u; }
        return h ? h : 1u;
        // never return 0 so we can use 0 as “unset”
    }

    // -------------------------
    // BBox + row popcounts
    // -------------------------
    bool DestructibleTileSystem::BBoxFrontiersWithThreshold(const std::vector<uint32_t>& words,
        int W, int H, const ConnCfg& cfg,
        int& bx0, int& by0, int& bx1, int& by1,
        int& topY, int& botY,
        std::vector<int>& rowPop)
    {
        bx0 = W; by0 = H; bx1 = -1; by1 = -1;
        rowPop.assign(H, 0);

        for (int y = 0; y < H; ++y)
        {
            int c = 0;
            for (int x = 0; x < W; ++x)
            {
                if (ReadBitPackedAt(words, W, H, x, y, cfg)) {
                    ++c;
                    if (x < bx0) bx0 = x;
                    if (x > bx1) bx1 = x;
                    if (y < by0) by0 = y;
                    if (y > by1) by1 = y;
                }
            }
            rowPop[y] = c;
        }
        if (!(bx1 >= bx0 && by1 >= by0)) return false;

        const int minPop = MIN_ROW_POP(bx1 - bx0 + 1);
        topY = -1; botY = -1;

        for (int y = by0; y <= by1; ++y)
            if (rowPop[y] >= minPop) { topY = y; break; }

        for (int y = by1; y >= by0; --y)
            if (rowPop[y] >= minPop) { botY = y; break; }

        return (topY != -1 && botY != -1 && topY <= botY);
    }

    // -------------------------
    // Connectivity (thresholded)
    // -------------------------

    bool DestructibleTileSystem::AnyPathConnectedBBoxThresholded(const std::vector<uint32_t>& words,
        int W, int H, const ConnCfg& cfg,
        int bx0, int by0, int bx1, int by1,
        int topY, int botY,
        int& outTopY, int& outBotY)
    {
        outTopY = topY; outBotY = botY;
        if (topY == botY) return (topY != -1);

        const int BW = bx1 - bx0 + 1;
        const int BH = by1 - by0 + 1;
        std::vector<uint8_t> vis(size_t(BW) * size_t(BH), 0);

        auto LIDX = [&](int x, int y)->size_t { return size_t(y - by0) * size_t(BW) + size_t(x - bx0); };

        std::queue<std::pair<int, int>> q;
        for (int x = bx0; x <= bx1; ++x)
        {
            if (ReadBitPackedAt(words, W, H, x, topY, cfg))
            {
                vis[LIDX(x, topY)] = 1;
                q.emplace(x, topY);
            }
        }

        if (USE_8_CONNECTED())
        {
            static const int DX8[8] = { +1,-1,0,0, +1,+1,-1,-1 };
            static const int DY8[8] = { 0, 0,+1,-1, +1,-1,+1,-1 };
            while (!q.empty()) {
                auto [cx, cy] = q.front(); q.pop();
                if (cy == botY) return true;
                for (int k = 0; k < 8; ++k)
                {
                    int nx = cx + DX8[k], ny = cy + DY8[k];
                    if (nx < bx0 || nx > bx1 || ny < by0 || ny > by1) continue;
                    if (!ReadBitPackedAt(words, W, H, nx, ny, cfg)) continue;
                    size_t li = LIDX(nx, ny);
                    if (vis[li]) continue;
                    vis[li] = 1; q.emplace(nx, ny);
                }
            }
        }
        else
        {
            static const int DX4[4] = { +1,-1,0,0 };
            static const int DY4[4] = { 0, 0,+1,-1 };
            while (!q.empty())
            {
                auto [cx, cy] = q.front(); q.pop();
                if (cy == botY) return true;
                for (int k = 0; k < 4; ++k)
                {
                    int nx = cx + DX4[k], ny = cy + DY4[k];
                    if (nx < bx0 || nx > bx1 || ny < by0 || ny > by1) continue;
                    if (!ReadBitPackedAt(words, W, H, nx, ny, cfg)) continue;
                    size_t li = LIDX(nx, ny);
                    if (vis[li]) continue;
                    vis[li] = 1; q.emplace(nx, ny);
                }
            }
        }
        return false;
    }


    bool DestructibleTileSystem::HasLostTooMuchArea(
        int currentAlive,
        int originalAlive,
        float minRemainingRatio)
    {
        if (originalAlive <= 0)
            return false;

        const float ratio = float(currentAlive) / float(originalAlive);
        return ratio <= minRemainingRatio;
    }

    bool DestructibleTileSystem::HasWideDestroyedBand(
        const std::vector<int>& rowPop,
        int topY,
        int botY,
        int tileAliveWidth,
        float maxAliveRatio,
        int minConsecutiveRows)
    {
        int run = 0;

        for (int y = topY + 1; y < botY; ++y)
        {
            const float aliveRatio = float(rowPop[y]) / float(tileAliveWidth);

            if (aliveRatio <= maxAliveRatio)
            {
                ++run;

                if (run >= minConsecutiveRows)
                    return true;
            }
            else
            {
                run = 0;
            }
        }

        return false;
    }

    int DestructibleTileSystem::CountAlivePixelsNearOriginalBase(
        const std::vector<int>& rowPop,
        int baselineBotY,
        int baseBandRows)
    {
        if (rowPop.empty())
            return 0;

        const int maxY = int(rowPop.size()) - 1;
        baselineBotY = std::clamp(baselineBotY, 0, maxY);

        const int y0 = std::max(0, baselineBotY - baseBandRows + 1);
        const int y1 = baselineBotY;

        int aliveInBaseBand = 0;

        for (int y = y0; y <= y1; ++y)
            aliveInBaseBand += rowPop[y];

        return aliveInBaseBand;
    }


    bool DestructibleTileSystem::HasWideEnoughBaseSupport(
        const std::vector<uint32_t>& words,
        int W,
        int H,
        const ConnCfg& cfg,
        int y0,
        int y1,
        int minX,
        int maxX,
        float minWidthRatio)
    {
        int minAliveX = W;
        int maxAliveX = -1;

        y0 = std::clamp(y0, 0, H - 1);
        y1 = std::clamp(y1, 0, H - 1);

        for (int y = y0; y <= y1; ++y)
        {
            for (int x = minX; x <= maxX; ++x)
            {
                if (!ReadBitPackedAt(words, W, H, x, y, cfg))
                    continue;

                minAliveX = std::min(minAliveX, x);
                maxAliveX = std::max(maxAliveX, x);
            }
        }

        if (maxAliveX < minAliveX)
            return false;

        const int aliveWidth = maxAliveX - minAliveX + 1;
        const int requiredWidth = int(float(maxX - minX + 1) * minWidthRatio);

        return aliveWidth >= requiredWidth;
    }

    bool DestructibleTileSystem::HasAlivePixelsNearOriginalBase(const std::vector<int>& rowPop, int baselineBotY,
        int baseBandRows, int minAlivePixels)
    {
        if (baselineBotY < 0)
            return true;

        const int y0 = std::max(0, baselineBotY - baseBandRows + 1);
        const int y1 = baselineBotY;

        int aliveInBaseBand = 0;

        for (int y = y0; y <= y1; ++y)
        {
            aliveInBaseBand += rowPop[y];
        }

        return aliveInBaseBand >= minAlivePixels;
    }


    bool DestructibleTileSystem::HasMultipleAliveComponents(const std::vector<uint32_t>& words, int W, int H,
        const ConnCfg& cfg, int bx0, int by0, int bx1, int by1, int minComponentPixels)
    {
        const int BW = bx1 - bx0 + 1;
        const int BH = by1 - by0 + 1;

        std::vector<uint8_t> visited(size_t(BW) * size_t(BH), 0);

        auto LIDX = [&](int x, int y) -> size_t
            {
                return size_t(y - by0) * size_t(BW) + size_t(x - bx0);
            };

        static const int DX4[4] = { +1, -1, 0, 0 };
        static const int DY4[4] = { 0, 0, +1, -1 };

        int largeComponents = 0;

        for (int y = by0; y <= by1; ++y)
        {
            for (int x = bx0; x <= bx1; ++x)
            {
                if (visited[LIDX(x, y)])
                    continue;

                if (!ReadBitPackedAt(words, W, H, x, y, cfg))
                    continue;

                int count = 0;
                std::queue<std::pair<int, int>> q;

                visited[LIDX(x, y)] = 1;
                q.emplace(x, y);

                while (!q.empty())
                {
                    auto [cx, cy] = q.front();
                    q.pop();

                    ++count;

                    for (int k = 0; k < 4; ++k)
                    {
                        int nx = cx + DX4[k];
                        int ny = cy + DY4[k];

                        if (nx < bx0 || nx > bx1 || ny < by0 || ny > by1)
                            continue;

                        size_t li = LIDX(nx, ny);

                        if (visited[li])
                            continue;

                        if (!ReadBitPackedAt(words, W, H, nx, ny, cfg))
                            continue;

                        visited[li] = 1;
                        q.emplace(nx, ny);
                    }
                }

                if (count >= minComponentPixels)
                {
                    ++largeComponents;

                    if (largeComponents >= 2)
                        return true;
                }
            }
        }

        return false;
    }


    bool DestructibleTileSystem::HasDeadRowGapInBBoxThresholded(const std::vector<int>& rowPop,
        int topY, int botY)
    {
        const int needRows = GAP_MIN_CONSEC_ROWS();
        const int deadPop = GAP_ROW_POP_THRESHOLD();
        int run = 0;
        for (int y = topY + 1; y < botY; ++y) {
            if (rowPop[y] <= deadPop) { if (++run >= needRows) return true; }
            else run = 0;
        }
        return false;
    }

    // -------------------------
    // Frontier reach + cut computation
    // -------------------------

    template<bool LSB_FIRST>
    Frontier    DestructibleTileSystem::FloodReachY(const std::vector<uint32_t>& aliveWords,
            int W, int H, int bx0, int by0, int bx1, int by1, const std::vector<std::pair<int, int>>& seeds)
    {
        const int wordsPerRow = (W + 31) / 32;
        std::vector<uint32_t> visited((H * wordsPerRow), 0u);
        auto testAlive = LSB_FIRST ? testBitLSB : testBitMSB;

        auto testVisited = [&](int x, int y) -> bool {
            const int idx = y * wordsPerRow + (x >> 5);
            const uint32_t bit = 1u << (x & 31);
            return (visited[idx] & bit) != 0;
            };
        auto setVisited = [&](int x, int y) {
            const int idx = y * wordsPerRow + (x >> 5);
            const uint32_t bit = 1u << (x & 31);
            visited[idx] |= bit;
            };

        std::vector<std::pair<int, int>> q;
        q.reserve((bx1 - bx0 + 1) * (by1 - by0 + 1) / 8);

        for (auto [sx, sy] : seeds) {
            if (sx < bx0 || sx > bx1 || sy < by0 || sy > by1) continue;
            if (!testAlive(aliveWords.data(), W, sx, sy)) continue;
            if (testVisited(sx, sy)) continue;
            setVisited(sx, sy);
            q.emplace_back(sx, sy);
        }

        int minY = INT_MAX, maxY = INT_MIN;
        const int dirs[4][2] = { {+1,0},{-1,0},{0,+1},{0,-1} };
        while (!q.empty()) {
            auto [x, y] = q.back(); q.pop_back();
            if (y < minY) minY = y;
            if (y > maxY) maxY = y;

            for (auto& d : dirs) {
                int nx = x + d[0], ny = y + d[1];
                if (nx < bx0 || nx > bx1 || ny < by0 || ny > by1) continue;
                if (!testAlive(aliveWords.data(), W, nx, ny)) continue;
                if (testVisited(nx, ny)) continue;
                setVisited(nx, ny);
                q.emplace_back(nx, ny);
            }
        }
        if (minY == INT_MAX) return { +INT_MAX, -INT_MIN }; // none
        return { minY, maxY };
    }

    void DestructibleTileSystem::BuildFrontierSeeds(const std::vector<int>& rowPop, int bx0, int by0, int bx1,
        int by1, int minRowPop, std::vector<std::pair<int, int>>& topSeeds, std::vector<std::pair<int, int>>& botSeeds)
    {
        int topY = -1, botY = -1;
        for (int y = by0; y <= by1; ++y) { if (rowPop[y] >= minRowPop) { topY = y; break; } }
        for (int y = by1; y >= by0; --y) { if (rowPop[y] >= minRowPop) { botY = y; break; } }

        if (topY == -1) topY = by0;
        if (botY == -1) botY = by1;

        const int step = std::max(1, (bx1 - bx0 + 1) / 16);
        for (int x = bx0; x <= bx1; x += step) topSeeds.emplace_back(x, topY);
        for (int x = bx0; x <= bx1; x += step) botSeeds.emplace_back(x, botY);
    }

    bool DestructibleTileSystem::ComputeCutYTrueConnectivity(const std::vector<uint32_t>& aliveWords,
        int W, int H, bool lsbFirst,
        int bx0, int by0, int bx1, int by1,
        const std::vector<int>& rowPop,
        int& outCutY)
    {
        EE_PROFILE_FUNCTION();

        const int bboxW = bx1 - bx0 + 1;
        const int minRowPop = std::max(1, (int)std::ceil(bboxW * 0.02f)); // ~2%

        std::vector<std::pair<int, int>> topSeeds, botSeeds;
        BuildFrontierSeeds(rowPop, bx0, by0, bx1, by1, minRowPop, topSeeds, botSeeds);

        Frontier topReach, botReach;
        if (lsbFirst)
        {
            topReach = FloodReachY<true >(aliveWords, W, H, bx0, by0, bx1, by1, topSeeds);
            botReach = FloodReachY<true >(aliveWords, W, H, bx0, by0, bx1, by1, botSeeds);
        }
        else
        {
            topReach = FloodReachY<false>(aliveWords, W, H, bx0, by0, bx1, by1, topSeeds);
            botReach = FloodReachY<false>(aliveWords, W, H, bx0, by0, bx1, by1, botSeeds);
        }

        if (topReach.y0 == +INT_MAX || botReach.y0 == +INT_MAX)
        {
            outCutY = (by0 + by1) / 2;
            return true;
        }

        // Connected => overlap/touch in Y
        if (topReach.y1 >= botReach.y0) return false;

        const int lo = topReach.y1;
        const int hi = botReach.y0;
        int bestY = -1, bestVal = INT_MAX;
        for (int y = lo + 1; y <= hi - 1; ++y) {
            int v = rowPop[y];
            if (v < bestVal) { bestVal = v; bestY = y; }
            if (v == 0) { bestY = y; break; }
        }
        outCutY = (bestY >= 0) ? bestY : ((lo + hi) / 2);
        return true;
    }

    // -------------------------
    // Simple physics helper
    // -------------------------

    float DestructibleTileSystem::DurationFromCutY(int cutY, int tileH_px,
        float pixelWorld, float gravityMag, bool isTopPiece)
    {
        const int pieceHeightPx = isTopPiece ? cutY : (tileH_px - cutY);
        const float d = glm::max(0.0f, float(pieceHeightPx) * pixelWorld);

        const float g = glm::max(1e-3f, gravityMag);
        float t = std::sqrt(2.0f * d / g);

        const float kStyle = 0.9f;
        t *= kStyle;

        const float tMin = 0.20f;
        const float tMax = 1.20f;
        return glm::clamp(t, tMin, tMax);
    }

    void DestructibleTileSystem::InitDestructableSystem(Scene* scene)
    {

        m_roofAccess.Init(scene);

        Engine::RoofSystemConfig cfg;
        cfg.minSupportsPerComponent = 2;

       // m_roofSystem.Init(scene, &m_roofAccess, cfg);

        m_tileStabilitySystem.InitTileStabilitySystem(scene);
    }

    // =========================
    // Public: OnTilesUpdated
    // =========================
    void DestructibleTileSystem::OnTilesUpdated(Scene* scene, float deltaTime)
    {
        EE_PROFILE_FUNCTION();

        m_roofSystem.Update(deltaTime);
        m_tileStabilitySystem.Update(deltaTime);

        const auto& dirtyTiles = Engine::TileBlockedMaskCPU::DirtyTileRuntime;
        if (dirtyTiles.empty())
        {
            return;
        }

        const int tilePixelWidth = TILE_PIXEL_WIDTH;
        const int tilePixelHeight = TILE_PIXEL_HEIGHT;
        const size_t expectedWordCount = (size_t(tilePixelWidth) * size_t(tilePixelHeight) + 31) / 32;

        Ref<VulkanBindlessDescriptorSetRenderer>& bindlessRenderer = VulkanRenderer2D::GetBindlessDescriptorSetRenderer();

        for (const auto& dirtyTile : dirtyTiles)
        {
            const uint32_t slot = dirtyTile.slot;

            if (slot >= m_connCfg.size())
            {
                m_connCfg.resize(slot + 1);
            }

            if (slot >= m_initialized.size())
            {
                m_initialized.resize(slot + 1, 0);
            }

            if (slot >= m_topBottomConnected.size())
            {
                m_topBottomConnected.resize(slot + 1, 0);
            }

            if (slot >= m_aliveCount.size())
            {
                m_aliveCount.resize(slot + 1, -1);
            }

            if (slot >= m_maskCrc.size())
            {
                m_maskCrc.resize(slot + 1, 0);
            }

            if (slot >= m_slotUID.size())
            {
                m_slotUID.resize(slot + 1, 0);
            }

            if (dirtyTile.aliveWords.size() < expectedWordCount)
            {
                continue;
            }

            const uint64_t currentUID = bindlessRenderer->GetTileUIDFromSlot(slot);
            if (currentUID == 0)
            {
                continue;
            }

            if (m_slotUID[slot] != currentUID)
            {
                m_slotUID[slot] = currentUID;
                m_connCfg[slot] = ConnCfg{};
                m_initialized[slot] = 0;
                m_topBottomConnected[slot] = 0;
                m_aliveCount[slot] = -1;
                m_maskCrc[slot] = 0;
            }

            const uint32_t currentMaskCrc = Crc32Words(dirtyTile.aliveWords);
            const bool maskChanged = currentMaskCrc != m_maskCrc[slot];
            m_maskCrc[slot] = currentMaskCrc;

            if (!maskChanged)
            {
                continue;
            }

            ConnCfg& connectionConfig = m_connCfg[slot];

            if (!connectionConfig.set)
            {
                const long long targetAliveCount = static_cast<long long>(dirtyTile.aliveCount);
                const long long oneIsAliveCount = CountAliveWithSense(dirtyTile.aliveWords, tilePixelWidth, tilePixelHeight, true, true);
                const long long zeroIsAliveCount = CountAliveWithSense(dirtyTile.aliveWords, tilePixelWidth, tilePixelHeight, true, false);

                connectionConfig.set = true;
                connectionConfig.lsbFirst = true;
                connectionConfig.oneIsAlive = std::llabs(oneIsAliveCount - targetAliveCount) <= std::llabs(zeroIsAliveCount - targetAliveCount);
                connectionConfig.lastAlive = int(targetAliveCount);
            }

            int boundsMinX = 0;
            int boundsMinY = 0;
            int boundsMaxX = 0;
            int boundsMaxY = 0;
            int topAliveY = 0;
            int bottomAliveY = 0;
            std::vector<int> alivePixelsPerRow;

            if (!BBoxFrontiersWithThreshold(dirtyTile.aliveWords, tilePixelWidth, tilePixelHeight, connectionConfig, boundsMinX, boundsMinY, boundsMaxX, boundsMaxY, topAliveY, bottomAliveY, alivePixelsPerRow))
            {
                m_initialized[slot] = 1;
                m_topBottomConnected[slot] = 0;
                m_aliveCount[slot] = 0;
                continue;
            }

            int totalAlivePixels = 0;
            for (int row = boundsMinY; row <= boundsMaxY; ++row)
            {
                totalAlivePixels += alivePixelsPerRow[row];
            }

            if (connectionConfig.baselineAlive < 0)
            {
                connectionConfig.baselineAlive = totalAlivePixels;
                connectionConfig.baselineTopY = topAliveY;
                connectionConfig.baselineBotY = bottomAliveY;
                connectionConfig.baselineBx0 = boundsMinX;
                connectionConfig.baselineBy0 = boundsMinY;
                connectionConfig.baselineBx1 = boundsMaxX;
                connectionConfig.baselineBy1 = boundsMaxY;
            }

            int connectedTopY = -1;
            int connectedBottomY = -1;

            const bool hasTopToBottomPath = AnyPathConnectedBBoxThresholded(dirtyTile.aliveWords, tilePixelWidth, tilePixelHeight, connectionConfig, boundsMinX, boundsMinY, boundsMaxX, boundsMaxY, topAliveY, bottomAliveY, connectedTopY, connectedBottomY);
            const bool hasDeadRowGap = HasDeadRowGapInBBoxThresholded(alivePixelsPerRow, topAliveY, bottomAliveY);
            const bool hasMultipleAliveComponents = HasMultipleAliveComponents(dirtyTile.aliveWords, tilePixelWidth, tilePixelHeight, connectionConfig, boundsMinX, boundsMinY, boundsMaxX, boundsMaxY, 32);

            const int aliveWidth = boundsMaxX - boundsMinX + 1;
            const bool hasWideDestroyedBand = HasWideDestroyedBand(alivePixelsPerRow, topAliveY, bottomAliveY, aliveWidth, 0.15f, 4);

            const int hardcodedBaselineBottomY = 184;
            const int baseBandRows = 24;
            const int minimumBasePixels = 64;

            const bool hasBasePixels = HasAlivePixelsNearOriginalBase(alivePixelsPerRow, hardcodedBaselineBottomY, baseBandRows, minimumBasePixels);

            const int minimumRequiredBaseY = 50;
            const bool lostHardcodedBase = bottomAliveY < minimumRequiredBaseY;

            const int minimumExpectedTopY = 30;
            const bool isFloatingTopPiece = topAliveY > minimumExpectedTopY;

            const bool shouldCollapseWholeRemainingPiece = !hasBasePixels || lostHardcodedBase || isFloatingTopPiece;

            const bool isDisconnected = !hasTopToBottomPath || hasDeadRowGap || hasMultipleAliveComponents || hasWideDestroyedBand || shouldCollapseWholeRemainingPiece;

            if (!isDisconnected)
            {
                m_initialized[slot] = 1;
                m_topBottomConnected[slot] = 1;
                m_aliveCount[slot] = totalAlivePixels;
                continue;
            }

            int cutY = -1;
            bool foundCut = false;

            if (shouldCollapseWholeRemainingPiece)
            {
                cutY = topAliveY + 1;
                foundCut = true;
            }
            else
            {
                foundCut = ComputeCutYTrueConnectivity(dirtyTile.aliveWords, tilePixelWidth, tilePixelHeight, connectionConfig.lsbFirst, boundsMinX, boundsMinY, boundsMaxX, boundsMaxY, alivePixelsPerRow, cutY);
            }

            if (!foundCut)
            {
                m_initialized[slot] = 1;
                m_topBottomConnected[slot] = 0;
                m_aliveCount[slot] = totalAlivePixels;
                continue;
            }

            if (!shouldCollapseWholeRemainingPiece)
            {
                const int minimumCutY = boundsMinY + 1;
                const int maximumCutY = boundsMaxY - 1;

                if (minimumCutY > maximumCutY)
                {
                    m_initialized[slot] = 1;
                    m_topBottomConnected[slot] = 0;
                    m_aliveCount[slot] = totalAlivePixels;
                    continue;
                }

                cutY = std::clamp(cutY, minimumCutY, maximumCutY);
            }

            Engine::Entity sourceEntity{};
            size_t sourceTileIndex = SIZE_MAX;
            bool foundSourceTile = false;

            scene->ForEach<Engine::TransformComponent, Engine::TileComponent, Engine::IDComponent>(
                [&](Engine::Entity entity, Engine::TransformComponent& transformComponent, Engine::TileComponent& tileComponent, Engine::IDComponent& idComponent)
                {
                    for (size_t tileIndex = 0; tileIndex < tileComponent.tiles.size(); ++tileIndex)
                    {
                        Engine::TileInfo& tileInfo = tileComponent.tiles[tileIndex];

                        if (tileInfo.IsSpawned)
                        {
                            continue;
                        }

                        if (tileComponent.tiles[tileIndex].UID != currentUID)
                        {
                            continue;
                        }

                        if (tileInfo.Slot != slot)
                        {
                            continue;
                        }

                        sourceEntity = entity;
                        sourceTileIndex = tileIndex;
                        foundSourceTile = true;

                        tileInfo.IsSupportingRoof = false;

                        const glm::vec2 tileWorldPosition = glm::vec2(transformComponent.Translation.x, transformComponent.Translation.y) + tileInfo.position;
                        const glm::ivec2 tileCell = IsoTileUtils::WorldToIsoCellInt(tileWorldPosition);

                        TileTypeKey tileTypeKey;
                        tileTypeKey.name = tileInfo.name;
                        tileTypeKey.category = tileInfo.Category;
                        tileTypeKey.direction = tileInfo.TileDirection;

                        uint16_t typeId = 0;
                        const TileDefinitionRegistry& tileDefinitionRegistry = AssetManager::GetTileDefinitions();

                        if (tileDefinitionRegistry.FindTypeId(tileTypeKey, typeId))
                        {
                            scene->GetCompactTileMap().ClearTileFlag(tileCell, typeId, tileInfo.floor, CompactTileFlags::CanCollapse);
                            scene->GetCompactTileMap().ClearTileFlag(tileCell, typeId, tileInfo.floor, CompactTileFlags::CanSupport);
                        }

                        m_tileStabilitySystem.NotifySupportLostAtCell(tileCell, tileInfo.floor);
                        return;
                    }
                });

            if (!foundSourceTile)
            {
                m_initialized[slot] = 1;
                m_topBottomConnected[slot] = 0;
                m_aliveCount[slot] = totalAlivePixels;
                continue;
            }

            const Engine::TransformComponent& sourceTransformComponent = sourceEntity.GetComponent<Engine::TransformComponent>();
            const Engine::TileComponent& sourceTileComponent = sourceEntity.GetComponent<Engine::TileComponent>();
            const Engine::TileInfo& sourceTileInfo = sourceTileComponent.tiles[sourceTileIndex];

            Engine::Entity newEntity = scene->CreateEntity(sourceTileInfo.name + " Top");
            EE_CORE_INFO("spawning new entity {} ", sourceTileInfo.name);
            Engine::IDComponent& newIdComponent = newEntity.GetComponent<Engine::IDComponent>();

            Engine::TransformComponent& newTransformComponent = newEntity.AddComponent<Engine::TransformComponent>();
            newTransformComponent.SetTransform(sourceTransformComponent.GetTransform());

            Engine::TileComponent& newTileComponent = newEntity.AddComponent<Engine::TileComponent>();

            const eTileCategory newTileCategory = eTileCategory::DynamicObjects;
            const uint64_t newTileUID = HashUtils::MakeTileUID((uint64_t)newIdComponent.ID, sourceTileInfo.position, float(TILE_SIZE), (uint32_t)newTileCategory, sourceTileInfo.TileDirection, sourceTileInfo.floor);

            const size_t textureByteCount = size_t(tilePixelWidth) * size_t(tilePixelHeight) * 4;
            std::vector<uint8_t> zeroColor(textureByteCount, 0);
            std::vector<uint8_t> zeroProps(textureByteCount, 0);

            VulkanContext* vulkanContext = VulkanContext::Get();
            VkCommandBuffer commandBuffer = vulkanContext->BeginSingleTimeCommands();

            uint32_t newSlot = bindlessRenderer->EnsureTileResidentFromRaw(newTileUID, zeroColor.data(), zeroColor.size(), zeroProps.data(), zeroProps.size(), commandBuffer);

            vulkanContext->EndSingleTimeCommands(commandBuffer);

            Engine::VulkanRenderer2D::RemoveTilePixels(slot, newSlot, dirtyTile.aliveWords, cutY);

            TileInfo newTileInfo = sourceTileInfo;
            newTileInfo.Slot = newSlot;
            newTileInfo.UID = newTileUID;
            newTileInfo.opaqueMax = glm::ivec2(0);
            newTileInfo.opaqueMin = glm::ivec2(0);
            newTileInfo.IsSpawned = true;
            newTileInfo.IsSupportingRoof = false;
            newTileInfo.Category = eTileCategory::DynamicObjects;
            newTileComponent.tiles.push_back(newTileInfo);

            connectionConfig.lastAlive = totalAlivePixels;

            const float pixelWorldSize = float(TILE_SIZE) / float(TILE_PIXEL_WIDTH);
            const float gravityMagnitude = 9.81f;
            const float simulateSeconds = DurationFromCutY(cutY, TILE_PIXEL_HEIGHT, pixelWorldSize, gravityMagnitude, true);

            const glm::vec2 initialVelocity = glm::vec2(+20.0f * pixelWorldSize, +40.0f * pixelWorldSize);
            const float initialAngularVelocity = glm::radians(120.0f);

            PhysicsUtils::AttachSimplePhysics(newEntity, initialVelocity, initialAngularVelocity, simulateSeconds, false, { 0.0f, -gravityMagnitude });

            m_initialized[slot] = 1;
            m_topBottomConnected[slot] = 0;
            m_aliveCount[slot] = totalAlivePixels;
        }
    }

}
