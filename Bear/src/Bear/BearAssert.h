#pragma once

#include "Log.h"

#ifdef BEAR_ENABLE_ASSERTS
#define BEAR_CORE_ASSERT(x, ...) { if(!(x)) { BEAR_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#define BEAR_CLIENT_ASSERT(x, ...) { if(!(x)) { BEAR_CLIENT_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#else
// 如果没有启用断言，则定义为一个空宏，这样在代码中使用断言时不会有任何影响
#define BEAR_CORE_ASSERT(x, ...)
#define BEAR_CLIENT_ASSERT(x, ...)
#endif