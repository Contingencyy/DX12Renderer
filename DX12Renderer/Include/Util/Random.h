#pragma once
#include <random>

class Random
{
public:
	static void Initialize()
	{
		s_RandomEngine.seed(std::random_device()());
	}

	/* Returns a random float between 0.0f and 1.0f */
	static inline float Float()
	{
		return (float)s_Distribution(s_RandomEngine) / (float)std::numeric_limits<uint32_t>::max();
	}

	/* Returns a random float between min and max */
	static inline float FloatRange(float min, float max)
	{
		return min + (float)s_Distribution(s_RandomEngine) / ((float)std::numeric_limits<uint32_t>::max() / (max - min));
	}

private:
	static std::mt19937 s_RandomEngine;
	static std::uniform_int_distribution<std::mt19937::result_type> s_Distribution;

};
