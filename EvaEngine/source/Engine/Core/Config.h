#pragma once
#include <cstdint>

//WINDOW
constexpr size_t WINDOW_WIDTH = 1600;
constexpr size_t WINDOW_HEIGHT = 900;

//TILES
constexpr size_t TILE_SIZE = 1;
constexpr size_t TILE_PIXEL_WIDTH = 128;
constexpr size_t TILE_PIXEL_HEIGHT = 256;
constexpr float GRID_TILE_W = (float)TILE_SIZE;
constexpr float GRID_TILE_H = GRID_TILE_W * 0.5f;
constexpr uint32_t TILE_FILE_MAGIC = 0x544C4532; // "TLE2"
constexpr uint32_t TILE_FILE_VERSION = 2;

// Engine constants
constexpr int MAX_FRAMES_IN_FLIGHT = 3;
constexpr size_t MAX_TEXTURES = 32;
constexpr size_t MAX_UI_TEXTURES = 32;
constexpr size_t MAX_ALBEDO_TEXTURES = 256;
constexpr size_t MAX_PROJECTILES = 64;
constexpr size_t MAX_COLLISION_ENTITIES = 64;
constexpr size_t MAX_COLLISION_RESULTS = 32;
constexpr size_t MAX_3D_INSTANCES = 1024;
constexpr size_t CHUNK_SIZE = 16;
constexpr size_t GRID_SUBDIVISIONS = 3;
constexpr size_t MAX_RESIDENT_LAYERS = 1024;
constexpr size_t MAX_SPRITESHEETS = 128;
constexpr size_t MAX_TILES_IN_CLEAR_BUFFER = 16;
constexpr float DIRTY_CELLS_PER_TILE = 16.0f;
constexpr uint32_t MAX_RECTS = 4096;
constexpr uint32_t WORD_BITS = 32;
constexpr size_t PLAYER_COUNT = 1;
constexpr size_t MAX_EXPLOSIONS = 32;
constexpr size_t MAX_MATERIALS = 32;


// lights
static constexpr uint32_t MAX_DIR_LIGHTS = 1;
static constexpr uint32_t MAX_POINT_LIGHTS = 64;
static constexpr uint32_t MAX_SPOT_LIGHTS = 16;


// 3D rendering types
using MeshId = uint32_t;
using SkeletonId = uint32_t;
using AnimClipId = uint32_t;
constexpr MeshId INVALID_MESH = 0xFFFFFFFFu;
constexpr SkeletonId INVALID_SKELETON = 0xFFFFFFFFu;
constexpr AnimClipId INVALID_CLIP = 0xFFFFFFFFu;
constexpr uint32_t WHOLE_MESH = 0xFFFFFFFFu;
