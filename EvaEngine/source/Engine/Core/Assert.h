#pragma once

#include "Engine/Core/Core.h" // for EE_DEBUGBREAK, EE_STRINGIFY_MACRO, etc.

namespace Engine
{
    enum class AssertChannel : uint8_t { Core, Client };

    
    void AssertFailNoMsg(AssertChannel channel,
        const char* expr,
        const char* file,
        int line);

    void AssertFailFmt(AssertChannel channel,
        const char* expr,
        const char* file,
        int line,
        const char* fmtStr, ...);
}

#ifdef EE_ENABLE_ASSERTS

#define EE_INTERNAL_ASSERT_IMPL(channel, check, ...) \
        do { \
            if (!(check)) { \
                Engine::AssertFailFmt(channel, EE_STRINGIFY_MACRO(check), __FILE__, __LINE__, __VA_ARGS__); \
                EE_DEBUGBREAK(); \
            } \
        } while (0)

#define EE_INTERNAL_ASSERT_NO_MSG(channel, check) \
        do { \
            if (!(check)) { \
                Engine::AssertFailNoMsg(channel, EE_STRINGIFY_MACRO(check), __FILE__, __LINE__); \
                EE_DEBUGBREAK(); \
            } \
        } while (0)

// Picks NO_MSG when only (check) is provided, otherwise uses FMT version.
#define EE_INTERNAL_ASSERT_GET_MACRO_NAME(_1,_2,NAME,...) NAME
#define EE_INTERNAL_ASSERT_GET_MACRO(...) \
        EE_EXPAND_MACRO(EE_INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, EE_INTERNAL_ASSERT_IMPL, EE_INTERNAL_ASSERT_NO_MSG))

#define EE_ASSERT(...) \
        EE_EXPAND_MACRO(EE_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(Engine::AssertChannel::Client, __VA_ARGS__))

#define EE_CORE_ASSERT(...) \
        EE_EXPAND_MACRO(EE_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(Engine::AssertChannel::Core, __VA_ARGS__))

#else

#define EE_ASSERT(...)
#define EE_CORE_ASSERT(...)

#endif
