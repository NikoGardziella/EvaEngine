#pragma once


#include <iostream>
#include <memory>
#include <utility>
#include <algorithm>
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <array>
#include <filesystem>

#include "Engine/Core/Log.h"
#include "Engine/Debug/Instrumentor.h"

#if EE_PLATFORM_WINDOWS
	#include <Windows.h>
#endif


#include <execution>
#include <future>

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
