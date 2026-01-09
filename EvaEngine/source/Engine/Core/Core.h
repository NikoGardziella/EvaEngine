#pragma once


#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/fmt/bundled/format.h>
#include <spdlog/fmt/bundled/base.h>
#include <spdlog/fmt/bundled/ostream.h>
#include <spdlog/fmt/bundled/core.h>
#include <spdlog/fmt/compile.h>
#include "Engine/Core/Log.h"
#include <filesystem>



#ifdef EE_PLATFORM_WINDOWS
    #if EE_DYNAMIC_LINK
        #ifdef EE_BUILD_DLL
            #define EE_API __declspec(dllexport)
        #else
            #define EE_API __declspec(dllimport)
        #endif
    #else
        #define EE_API
    #endif
#else
    #error Only Windows is supported!
#endif
#ifdef EE_DEBUG
#define EE_ENABLE_ASSERTS
#endif


#define EE_STRINGIFY_MACRO(x) #x
#define EE_EXPAND_MACRO(x) x

#ifdef EE_ENABLE_ASSERTS

// Ensures proper formatting and breaks execution on failure
#define EE_INTERNAL_ASSERT_IMPL(check, msg, ...) \
    do { \
        if (!(check)) { \
            std::string formattedMessage = fmt::format(FMT_STRING(msg), ##__VA_ARGS__); \
            EE_CORE_ERROR("Assertion failed: {}", formattedMessage); \
            EE_DEBUGBREAK(); \
        } \
    } while (0)

// Handles cases with or without a custom message
#define EE_INTERNAL_ASSERT_WITH_MSG(check, ...) EE_INTERNAL_ASSERT_IMPL(check, __VA_ARGS__)
#define EE_INTERNAL_ASSERT_NO_MSG(check) EE_INTERNAL_ASSERT_IMPL(check, "Assertion '{}' failed at {}:{}", \
        EE_STRINGIFY_MACRO(check), std::filesystem::path(__FILE__).filename().string(), __LINE__)

// Macro selection logic
#define EE_INTERNAL_ASSERT_GET_MACRO_NAME(arg1, arg2, macro, ...) macro
#define EE_INTERNAL_ASSERT_GET_MACRO(...) \
    EE_EXPAND_MACRO(EE_INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, \
        EE_INTERNAL_ASSERT_WITH_MSG, EE_INTERNAL_ASSERT_NO_MSG))

// Final macros
#define EE_ASSERT(...) EE_EXPAND_MACRO(EE_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(__VA_ARGS__))
#define EE_CORE_ASSERT(...) EE_EXPAND_MACRO(EE_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(__VA_ARGS__))

#else
#define EE_ASSERT(...)
#define EE_CORE_ASSERT(...)
#endif

// Debug break handling
#ifdef EE_PLATFORM_WINDOWS
#define EE_DEBUGBREAK() __debugbreak()
#else
#define EE_DEBUGBREAK() __builtin_trap()
#endif




#define FUNCTION_POINTER(name) ScriptableEntity* (*name)()

#define  EE_BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)

#define BIT(x) (1 << x)

//#define MAX_TEXTURES = 512

const int MAX_FRAMES_IN_FLIGHT = 3;

constexpr size_t TILE_SIZE = 1;
constexpr size_t MAX_TEXTURES = 32;
constexpr size_t MAX_UI_TEXTURES = 32;
constexpr size_t MAX_PROJECTILES = 64;
constexpr size_t MAX_COLLISION_ENTITIES = 64; // projectiles and player
constexpr size_t MAX_COLLISION_RESULTS = 32; 
constexpr size_t TILE_PIXEL_WIDTH = 128;
constexpr size_t TILE_PIXEL_HEIGHT = 256;

constexpr float GRID_TILE_W = (float)TILE_SIZE;           // diamond width
constexpr float GRID_TILE_H = GRID_TILE_W * 0.5f; // diamond height = 64

// if you modify this valu, there must be terrain tile in all chunks
// otherwise destruction and effect will break.
constexpr size_t CHUNK_SIZE = 16; // Tiles in a chunk

constexpr size_t GRID_SUBDIVISIONS = 3;
constexpr size_t MAX_RESIDENT_LAYERS = 1024;
constexpr size_t MAX_SPRITESHEETS = 128;
constexpr size_t MAX_TILES_IN_CLEAR_BUFFER = 16;




// ---- Dirty rect subdivision used by the shader (keep 16 here) ----
static constexpr float DIRTY_CELLS_PER_TILE = 16.0f;
constexpr uint32_t MAX_RECTS = 4096; // compute binding=4
static constexpr uint32_t WORD_BITS = 32;

constexpr size_t PLAYER_COUNT = 1; // Tiles in a chunk

// GRID dont forget to modify in shader as well 
//effects
constexpr size_t MAX_EXPLOSIONS = 32;

 // 3D rendering
using MeshId = uint32_t;
using SkeletonId = uint32_t;
using AnimClipId = uint32_t;
static constexpr MeshId      INVALID_MESH = 0xFFFFFFFFu;
static constexpr SkeletonId  INVALID_SKELETON = 0xFFFFFFFFu;
static constexpr AnimClipId  INVALID_CLIP = 0xFFFFFFFFu;
static constexpr uint32_t WHOLE_MESH = 0xFFFFFFFFu;

constexpr size_t MAX_3D_INSTANCES = 1024;
constexpr size_t MAX_ALBEDO_TEXTURES = 256;
constexpr size_t MAX_MATERIALS = 32;


namespace Engine {

    template<typename T>
    using Scope = std::unique_ptr<T>;

    template<typename T>
    using Ref = std::shared_ptr<T>;

}
