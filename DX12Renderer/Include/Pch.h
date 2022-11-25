#pragma once

/*

	STL includes

*/
#include <assert.h>
#include <memory>
#include <chrono>
#include <string>
#include <queue>
#include <vector>
#include <array>
#include <unordered_map>
#include <algorithm>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <limits>

/*

	WIN32 includes/macros

*/
#include "WinIncludes.h"
#define DX_CALL(hr) if (hr != S_OK) throw std::exception()

/*

	Extern includes

*/
#include <glm/glm/glm.hpp>
#include <glm/glm/gtx/compatibility.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtx/matrix_decompose.hpp>
#include <glm/glm/gtc/quaternion.hpp>
#include <glm/glm/gtc/type_ptr.hpp>

/*

	Project defines and macros

*/
#define ASSERT(x, y) if (!(x)) LOG_ERR(y); assert(x)
#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)
#define BYTE_PADDING(x) unsigned char CONCAT(_padding_, __LINE__)[x] = {0}
#define MEGABYTE(x) x * 1024 * 1024
#define KILOBYTE(x) x * 1024

constexpr bool GPU_VALIDATION_ENABLED = false;

/*

	Since distance attenuation is a quadratic function, we can calculate a light's range based on its constant, linear, and quadratic falloff.
	The light range epsilon defines the attenuation value that is required before the light source will be ignored for further calculations in the shaders.
	We calculate it once on the CPU so that the GPU does not have to do it for every fragment/pixel. Only applies to point and spotlights.
	In the shader we need to rescale the attenuation value to account for this cutoff, otherwise the cutoff will be visible.

*/
constexpr float LIGHT_RANGE_EPSILON = 1.0f / 0.01f;

/*

	Project includes

*/
#include "Util/MathHelper.h"
#include "Util/Random.h"
#include "Util/Logger.h"
#include "Util/Profiler.h"
#include "Util/StringHelper.h"
#include "Util/ThreadSafeQueue.h"
