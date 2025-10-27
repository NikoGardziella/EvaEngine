#pragma once
#include <vector>


namespace Engine
{

    
    class DestructibleTileSystem {
    public:
        void Init(uint32_t maxSlots)
        {
            m_topBottomConnected.assign(maxSlots, false);
            m_initialized.assign(maxSlots, false);
        }

        bool TopBottomConnectedBits_Ref(const std::vector<uint8_t>& aliveBits, int W, int H);

        // Call this once per tile *after* you merge the GPU delta into aliveBits
        void OnTileUpdated();

    private:
        // returns true if there exists a 4-connected path of alive pixels from y=0 to y=H-1
        static bool TopBottomConnectedBits(const std::vector<uint8_t>& aliveBits, int W, int H);

    private:
        std::vector<bool> m_topBottomConnected; // indexed by slot
        std::vector<bool> m_initialized;        // indexed by slot
    };
}

