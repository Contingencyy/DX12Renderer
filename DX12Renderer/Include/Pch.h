#pragma once

/*

	STL includes

*/
#include <assert.h>
#include <memory>
#include <chrono>
#include <string>

/*

	WIN32 includes/macros

*/
#include "WinIncludes.h"
#define DX_CALL(x) if (x != S_OK) throw std::exception()

/*

	Extern includes

*/
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>

/*

	Project defines and macros

*/
#define ASSERT(x, y) if (!x) Logger::Log(y, Logger::Severity::ERR); assert(x)

/*

	Project includes

*/
#include "MathHelper.h"
#include "Random.h"
#include "Logger.h"
