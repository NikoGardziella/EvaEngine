#include "pch.h"
#include "DestrutibleTileSystem.h"
#include <Engine/Map/Grid/TileCollisionMask.h>
#include <Engine/Renderer/VulkanRenderer2D.h>
#include <Engine/Math/HashUtils.h>
#include <Engine/Scene/Components/Physics/PhysicsComponent.h>

#include <queue>
#include <cstdint>
#include <algorithm>
#include <cmath>

#include <Engine/Scene/Components/Render/TileComponent.h>

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
    void DestructibleTileSystem::AttachSimplePhysics(Engine::Entity e, glm::vec2 initialVelocity,
        float initialSpinRadPerSec, float simulateSeconds, bool destroyOnFinish, glm::vec2 gravity)
    {
        auto& pc = e.AddComponent<PhysicsComponent>();
        pc.velocity = initialVelocity;
        pc.angularVelocity = initialSpinRadPerSec;
        pc.duration = simulateSeconds;
        pc.timeLeft = simulateSeconds;
        pc.gravity = gravity;
        pc.active = true;
        pc.destroyOnFinish = destroyOnFinish;
    }

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

    // =========================
    // Public: OnTilesUpdated
    // =========================
    void DestructibleTileSystem::OnTilesUpdated(Scene* scene)
    {
        EE_PROFILE_FUNCTION();

        const auto& tiles = Engine::TileBlockedMaskCPU::DirtyTileRuntime;
        if (tiles.empty()) return;

        const int W = TILE_PIXEL_WIDTH;
        const int H = TILE_PIXEL_HEIGHT;
        const size_t expectedWords = (size_t(W) * size_t(H) + 31) / 32;

        for (const auto& tr : tiles)
        {
            const uint32_t slot = tr.slot;

            // Ensure per-slot storage
            if (slot >= m_connCfg.size())            m_connCfg.resize(slot + 1);
            if (slot >= m_initialized.size())        m_initialized.resize(slot + 1, 0);
            if (slot >= m_topBottomConnected.size()) m_topBottomConnected.resize(slot + 1, 0);
            if (slot >= m_aliveCount.size())         m_aliveCount.resize(slot + 1, -1);
            if (slot >= m_maskCrc.size())            m_maskCrc.resize(slot + 1, 0);

            if (tr.aliveWords.size() < expectedWords) continue;

            // Change detection
            const uint32_t curCrc = Crc32Words(tr.aliveWords);
            const bool changed = (curCrc != m_maskCrc[slot]);
            m_maskCrc[slot] = curCrc;
            if (!changed) continue;

            // Calibrate bit sense once
            ConnCfg& cfg = m_connCfg[slot];
            if (!cfg.set)
            {
                const long long t = static_cast<long long>(tr.aliveCount);
                const long long c1 = CountAliveWithSense(tr.aliveWords, W, H, /*lsbFirst*/true,  /*oneIsAlive*/true);
                const long long c0 = CountAliveWithSense(tr.aliveWords, W, H, /*lsbFirst*/true,  /*oneIsAlive*/false);
                const bool pickOneIsAlive = (std::llabs(c1 - t) <= std::llabs(c0 - t));
                cfg.set = true;
                cfg.lsbFirst = true;      // if you also need to calibrate msb/lsb, add logic here
                cfg.oneIsAlive = pickOneIsAlive;
                cfg.lastAlive = int(t);
            }

            // Build bbox + frontiers
            int bx0, by0, bx1, by1, topY, botY;
            std::vector<int> rowPop;
            if (!BBoxFrontiersWithThreshold(tr.aliveWords, W, H, cfg,
                bx0, by0, bx1, by1, topY, botY, rowPop))
            {
                m_initialized[slot] = 1;
                m_topBottomConnected[slot] = 0;
                m_aliveCount[slot] = 0;
                continue;
            }

            // Connectivity and gap checks
            int tY = -1, bY = -1;
            const bool connected_any = AnyPathConnectedBBoxThresholded(tr.aliveWords, W, H, cfg,
                bx0, by0, bx1, by1, topY, botY, tY, bY);
            const bool gap = HasDeadRowGapInBBoxThresholded(rowPop, topY, botY);
            const bool disconnected = (!connected_any) || gap;

            // total alive within bbox
            int totalAlive = 0;
            for (int y = by0; y <= by1; ++y) totalAlive += rowPop[y];

            if (disconnected)
            {
                int cutY = -1;
                bool got = ComputeCutYTrueConnectivity(tr.aliveWords, W, H, cfg.lsbFirst,
                    bx0, by0, bx1, by1, rowPop, cutY);
                if (got)
                {
                    cutY = std::clamp(cutY, by0 + 1, by1 - 1);

                    // Find owner tile entity for this slot
                    Engine::Entity srcEntity{};
                    size_t srcTileIndex = SIZE_MAX;
                    bool found = false;

                    scene->ForEachConst<Engine::TransformComponent, Engine::TileComponent, Engine::IDComponent>(
                        [&](Engine::Entity e,
                            const Engine::TransformComponent& xform,
                            const Engine::TileComponent& tc,
                            const Engine::IDComponent& idc)
                        {
                            for (size_t i = 0; i < tc.tiles.size(); ++i) {
                                if (tc.tiles[i].Slot == slot) {
                                    srcEntity = e;
                                    srcTileIndex = i;
                                    found = true;
                                    return;
                                }
                            }
                        });

                    if (!found) {
                        EE_CORE_WARN("Split: owner entity for slot {} not found", slot);
                    }
                    else {
                        const Engine::TransformComponent& srcXf = srcEntity.GetComponent<Engine::TransformComponent>();
                        const Engine::TileComponent& srcTc = srcEntity.GetComponent<Engine::TileComponent>();
                        const Engine::IDComponent& srcId = srcEntity.GetComponent<Engine::IDComponent>();
                        const Engine::TileInfo& srcTi = srcTc.tiles[srcTileIndex];

                        // Create a new entity for the top piece (new slot is allocated inside RemoveTilePixels path)
                        Engine::Entity newEntity = scene->CreateEntity(srcTi.name + " Top");
                        Engine::IDComponent& idNew = newEntity.GetComponent<Engine::IDComponent>();
                        Engine::TransformComponent& xfNew = newEntity.AddComponent<Engine::TransformComponent>();
                        xfNew.SetTransform(srcXf.GetTransform()); // copy transform

                        Engine::TileComponent& tcNew = newEntity.AddComponent<Engine::TileComponent>();


                        uint64_t newTileUID = HashUtils::MakeTileUID((uint64_t)idNew.ID, srcTi.position, float(TILE_SIZE));
                        const uint32_t W = TILE_PIXEL_WIDTH;
                        const uint32_t H = TILE_PIXEL_HEIGHT;
                        const size_t bytes = size_t(W) * size_t(H) * 4; std::vector<uint8_t> zeroColor(bytes, 0);
                        std::vector<uint8_t> zeroProps(bytes, 0);

                        VulkanContext* ctx = VulkanContext::Get();
                        VkCommandBuffer cb = ctx->BeginSingleTimeCommands();
                        Ref<VulkanBindlessDescriptorSetRenderer>& bindless = VulkanRenderer2D::GetBindlessDescriptorSetRenderer();
                        VkImage colorImg = bindless->GetColorImageArray();
                        VkImage propsImg = bindless->GetPropsArrayImage();
                        const uint32_t copyY = std::clamp(cutY, 0, (int)H);
                        const uint32_t copyH = (copyY < H) ? (H - copyY) : 0u;
                        // helper: per-layer barrier
                        uint32_t newSlot = bindless->EnsureTileResidentFromRaw(newTileUID, zeroColor.data(), zeroColor.size(), zeroProps.data(), zeroProps.size(), cb);
                        ctx->EndSingleTimeCommands(cb);


                        
                        Engine::VulkanRenderer2D::RemoveTilePixels(slot, newSlot, tr.aliveWords, cutY);
                        
                        TileInfo newTIle = srcTi;
                        newTIle.Slot = newSlot;
                        newTIle.UID  = newTileUID;
                        tcNew.tiles.push_back(newTIle);

                        cfg.lastAlive = totalAlive;

                        // Physics kick for the top piece
                        const float pxW = float(TILE_SIZE) / float(TILE_PIXEL_WIDTH);
                        const float gravityMag = 9.81f;

                        float simulateSeconds = DurationFromCutY(cutY, TILE_PIXEL_HEIGHT, pxW, gravityMag, /*isTopPiece=*/true);

                        glm::vec2 v0 = glm::vec2(+20.0f * pxW, +40.0f * pxW); // tweak
                        float     w0 = glm::radians(120.0f);

                        AttachSimplePhysics(newEntity, v0, w0, simulateSeconds,
                            /*destroyOnFinish*/false, { 0.f, -gravityMag });

                    }
                }
            }

            // Update state
            m_initialized[slot] = 1;
            m_topBottomConnected[slot] = disconnected ? 0 : 1;
            m_aliveCount[slot] = totalAlive;
        }
    }

}
