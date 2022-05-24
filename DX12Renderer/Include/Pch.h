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

/*

	WIN32 includes/macros

*/
#include "WinIncludes.h"
#define DX_CALL(x) if (x != S_OK) throw std::exception()

/*

	Extern includes

*/
#include <glm/glm/glm.hpp>
#include <glm/glm/gtx/compatibility.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>

/*

	Project defines and macros

*/
#define ASSERT(x, y) if (!x) Logger::Log(y, Logger::Severity::ERR); assert(x)

/*

	Project includes

*/
#include "Util/MathHelper.h"
#include "Util/Random.h"
#include "Util/Logger.h"
