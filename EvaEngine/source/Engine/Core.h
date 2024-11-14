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