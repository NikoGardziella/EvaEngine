#pragma once
#include <string>
#include "glm/glm.hpp"
#include <vector>

enum class eDetachTest { BreakBandY, PixelConnectivity };

struct DetachPiece
{
    std::string id;                 // "top"
    // Visuals
    glm::ivec4 uvPx;                // x,y,w,h in the atlas (piece sprite)
    glm::vec2  localOriginPx;       // pivot inside the tile (e.g., where it was attached)
    // Physics
    float      mass = 3.0f;
    enum { Box, Capsule } collider = Capsule;
    glm::vec2  colliderSizePx = { 8, 64 }; // r,h for capsule OR w,h for box
    // Detach rule
    eDetachTest test = eDetachTest::BreakBandY;
    int        breakYpx = 48;       // if a horizontal band around this Y gets cut -> detach
    int        bandHalfThicknessPx = 2; // size of the band for the cut test
    float      minCutCoverage = 0.7f;   // 70% of the band cut -> detach
    bool       oneShot = true;      // detach only once
    bool       hingesToBase = false;// if true, create joint instead of free fall
};

struct DestructibleTileDef
{
    std::vector<DetachPiece> pieces;
};