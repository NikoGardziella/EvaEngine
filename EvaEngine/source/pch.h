#pragma once

// STANDARD LIBRARY - Keep (rarely changes, big compilation cost)
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
#include <set>

// Parallelism/Threading
#include <execution>
#include <future>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

// ============================================================================
// PLATFORM SPECIFIC
// ============================================================================
#ifdef EE_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN  // Exclude rarely-used Windows headers
#define NOMINMAX             // Prevent Windows min/max macros
#include <Windows.h>
#endif

// ============================================================================
#include "Engine.h"
