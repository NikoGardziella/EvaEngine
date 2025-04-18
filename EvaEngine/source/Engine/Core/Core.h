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

//#include "Engine/Core/Log.h"

namespace Engine {

    template<typename T>
    using Scope = std::unique_ptr<T>;

    template<typename T>
    using Ref = std::shared_ptr<T>;

}
