#pragma once

#include <memory>

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

#ifdef EE_ENABLE_ASSERTS
    //#define EE_ASSERT(...) EE_EXPAND_MACRO(EE_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(, __VA_ARGS__))
    //#define EE_CORE_ASSERT(...) EE_EXPAND_MACRO(EE_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_CORE_, __VA_ARGS__))
  #define EE_ASSERT(...)
    #define EE_CORE_ASSERT(...)
#else
    #define EE_ASSERT(...)
    #define EE_CORE_ASSERT(...)
#endif

#define EE_EXPAND_MACRO(x) x

#define EE_INTERNAL_ASSERT_IMPL(type, check, msg, ...) \
    { \
        if (!(check)) { \
            EE_CORE_ERROR(msg, __VA_ARGS__); \
            EE_DEBUGBREAK(); \
        } \
    }

#define EE_INTERNAL_ASSERT_WITH_MSG(type, check, ...) \
    EE_INTERNAL_ASSERT_IMPL(type, check, "Assertion Failed: {0}", __VA_ARGS__)

#define EE_INTERNAL_ASSERT_NO_MSG(type, check) \
    EE_INTERNAL_ASSERT_IMPL(type, check, "Assertion Failed at {0}:{1}", __FILE__, __LINE__)

#define EE_INTERNAL_ASSERT_GET_MACRO_NAME(arg1, arg2, macro, ...) macro

#define EE_INTERNAL_ASSERT_GET_MACRO(...) \
    EE_EXPAND_MACRO(EE_INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, EE_INTERNAL_ASSERT_WITH_MSG, EE_INTERNAL_ASSERT_NO_MSG))

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
