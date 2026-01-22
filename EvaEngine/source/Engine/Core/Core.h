#pragma once

#include <memory>

// Platform detection
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

// Debug break
#ifdef EE_PLATFORM_WINDOWS
#define EE_DEBUGBREAK() __debugbreak()
#else
#define EE_DEBUGBREAK() __builtin_trap()
#endif

// Debug mode detection
#ifdef EE_DEBUG
#define EE_ENABLE_ASSERTS
#endif

// Macro utilities (used by Assert.h)
#define EE_STRINGIFY_MACRO(x) #x
#define EE_EXPAND_MACRO(x) x

// Utility macros
#define FUNCTION_POINTER(name) ScriptableEntity* (*name)()
#define EE_BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)
#define BIT(x) (1 << x)

// Smart pointer aliases
namespace Engine {
    template<typename T>
    using Scope = std::unique_ptr<T>;

    template<typename T>
    using Ref = std::shared_ptr<T>;
}