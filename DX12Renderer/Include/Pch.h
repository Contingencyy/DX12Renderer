#pragma once

/*

	STL includes

*/
#include <assert.h>
#include <memory>
#include <chrono>
#include <string>
#include <queue>
#include <unordered_map>
#include <algorithm>
#include <functional>

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

/*

	Project defines and macros

*/
#define ASSERT(x, y) if (!x) LOG_ERR(y); assert(x)

/*

	Project includes

*/
#include "Util/MathHelper.h"
#include "Util/Random.h"
#include "Util/Logger.h"
#include "Util/Profiler.h"
#include "Util/StringHelper.h"
