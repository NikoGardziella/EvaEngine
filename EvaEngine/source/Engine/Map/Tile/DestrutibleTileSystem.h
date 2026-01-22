#pragma once

#pragma once
#include <vector>
#include <queue>
#include <cstdint>
#include <algorithm>
#include <Engine/Renderer/Utils/DeltaBitReader.h>
#include <glm/glm.hpp>

namespace Engine
{

    struct ConnCfg {
        bool lsbFirst = true;   // bit index = (idx&31) by default (LSB-first)
        bool oneIsAlive = true;   // whether bit=1 means "alive" (will calibrate)
        bool set = false;
        int  lastAlive = -1;     // last measured alive count using chosen sense
    };
    struct CompSpan {
        bool connected;  // component spans bbox top-alive frontier -> bottom-alive frontier
        int  minY;       // component min Y (absolute tile y)
        int  maxY;       // component max Y
        int  topY;       // bbox top alive frontier (abs y), -1 if none
        int  botY;       // bbox bottom alive frontier (abs y), -1 if none
        int  totalAlive; // alive pixels inside bbox (using chosen sense)
    };
    struct Frontier 
    {
        int y0, y1;
    }; // inclusive

    class Entity;
    class Scene;
    class DestructibleTileSystem
    {

      


    public:
        void SpawnDetachedChunk(uint32_t slot, int cutY, const DirtyTileRuntime& tr, const ConnCfg& cfg);
        void OnTilesUpdated(Scene* scene);
        void Reset()
        {
            m_initialized.clear();
            m_topBottomConnected.clear();
            m_aliveCount.clear();
            m_maskCrc.clear();
            m_connCfg.clear();
        }

    private:
        // Per-slot config
       
        
        // State per slot
        std::vector<uint8_t>  m_initialized;         // 0/1
        std::vector<uint8_t>  m_topBottomConnected;  // 0/1 (component connectivity)
        std::vector<int>      m_aliveCount;          // last totalAlive we reported
        std::vector<uint32_t> m_maskCrc;             // CRC of aliveWords
        std::vector<ConnCfg>  m_connCfg;             // packing config per slot

    private:
        

        int MIN_ROW_POP(int W);

        int GAP_ROW_POP_THRESHOLD();

        int GAP_MIN_CONSEC_ROWS();

        bool USE_8_CONNECTED();

        bool ReadBitPackedAt(const std::vector<uint32_t>& words, int W, int H, int x, int y, const ConnCfg& cfg);

        int CountAliveWithSense(const std::vector<uint32_t>& words, int W, int H, bool lsbFirst, bool oneIsAlive);

        uint32_t Crc32Words(const std::vector<uint32_t>& v);

        bool BBoxFrontiersWithThreshold(const std::vector<uint32_t>& words, int W, int H, const ConnCfg& cfg, int& bx0, int& by0, int& bx1, int& by1, int& topY, int& botY, std::vector<int>& rowPop);

        bool AnyPathConnectedBBoxThresholded(const std::vector<uint32_t>& words, int W, int H, const ConnCfg& cfg, int bx0, int by0, int bx1, int by1, int topY, int botY, int& outTopY, int& outBotY);

        bool HasDeadRowGapInBBoxThresholded(const std::vector<int>& rowPop, int topY, int botY);

        void BuildFrontierSeeds(const std::vector<int>& rowPop, int bx0, int by0, int bx1, int by1, int minRowPop, std::vector<std::pair<int, int>>& topSeeds, std::vector<std::pair<int, int>>& botSeeds);

        bool ComputeCutYTrueConnectivity(const std::vector<uint32_t>& aliveWords, int W, int H, bool lsbFirst, int bx0, int by0, int bx1, int by1, const std::vector<int>& rowPop, int& outCutY);

        void AttachSimplePhysics(Engine::Entity e, glm::vec2 initialVelocity, float initialSpinRadPerSec, float simulateSeconds, bool destroyOnFinish, glm::vec2 gravity);

        float DurationFromCutY(int cutY, int tileH_px, float pixelWorld, float gravityMag, bool isTopPiece);

        template<bool LSB_FIRST>
        Frontier FloodReachY(const std::vector<uint32_t>& aliveWords, int W, int H, int bx0, int by0, int bx1, int by1, const std::vector<std::pair<int, int>>& seeds);

    };



}

