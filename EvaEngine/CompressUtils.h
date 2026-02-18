#pragma once
#include <vector>
#include "lz4.h"
#include "lz4hc.h"

class CompressUtils {

    static std::vector<uint8_t> ExtractSubRect(const std::vector<uint8_t>& fullData,
        int tileW, int tileH, int x0, int y0, int w, int h)
    {
        std::vector<uint8_t> sub(size_t(w) * size_t(h) * 4);
        for (int y = 0; y < h; ++y)
        {
            size_t srcOff = (size_t(y0 + y) * tileW + x0) * 4;
            size_t dstOff = size_t(y) * w * 4;
            memcpy(&sub[dstOff], &fullData[srcOff], size_t(w) * 4);
        }
        return sub;
    }

    static void InsertSubRect(std::vector<uint8_t>& fullData, const std::vector<uint8_t>& sub,
        int tileW, int tileH, int x0, int y0, int w, int h)
    {
        fullData.assign(size_t(tileW) * size_t(tileH) * 4, 0);
        for (int y = 0; y < h; ++y)
        {
            size_t srcOff = size_t(y) * w * 4;
            size_t dstOff = (size_t(y0 + y) * tileW + x0) * 4;
            memcpy(&fullData[dstOff], &sub[srcOff], size_t(w) * 4);
        }
    }

    static std::vector<uint8_t> CompressLZ4HC(const uint8_t* data, uint32_t size)
    {
        int maxSize = LZ4_compressBound(size);
        std::vector<uint8_t> compressed(maxSize);
        int compressedSize = LZ4_compress_HC(
            (const char*)data,
            (char*)compressed.data(),
            size,
            maxSize,
            LZ4HC_CLEVEL_DEFAULT);

        if (compressedSize <= 0)
        {
            EE_CORE_ERROR("TileStreaming: LZ4 HC compression failed");
            return {};
        }
        compressed.resize(compressedSize);
        return compressed;
    }

    static bool DecompressLZ4(const std::vector<uint8_t>& compressed,
        std::vector<uint8_t>& decompressed, uint32_t originalSize)
    {
        decompressed.resize(originalSize);
        int result = LZ4_decompress_safe(
            (const char*)compressed.data(),
            (char*)decompressed.data(),
            static_cast<int>(compressed.size()),
            originalSize);

        if (result < 0)
        {
            EE_CORE_ERROR("TileStreaming: LZ4 decompression failed");
            return false;
        }
        return true;
    }


};