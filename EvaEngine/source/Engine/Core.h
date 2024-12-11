#pragma once

#ifdef EE_PLATFORM_WINDOWS
	#ifdef EE_BUILD_DLL
		#define EE_API __declspec(dllexport)
	#else
		#define EE_API __declspec(dllimport)
	#endif
#else
	#error Only windows!
#endif

#ifdef HZ_ENABLE_ASSERTS

// Currently accepts at least the condition and one additional parameter (the message) being optional
	#define EE_ASSERT(...) EE_EXPAND_MACRO( EE_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_, __VA_ARGS__) )
	#define EE_CORE_ASSERT(...) EE_EXPAND_MACRO( EE_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_CORE_, __VA_ARGS__) )
#else
	#define EE_ASSERT(...)
	#define EE_CORE_ASSERT(...)
#endif



#define BIT(x) (1 << x)

#define EE_BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)