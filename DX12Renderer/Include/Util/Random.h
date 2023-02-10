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

	/* Returns a random uint32_t between 0 and 1 */
	static inline uint32_t Uint()
	{
		return (uint32_t)s_Distribution(s_RandomEngine) / std::numeric_limits<uint32_t>::max();
	}

	/* Returns a random uint32_t between min and max */
	static inline uint32_t UintRange(uint32_t min, uint32_t max)
	{
		return min + (uint32_t)s_Distribution(s_RandomEngine) / (std::numeric_limits<uint32_t>::max() / (max - min));
	}

private:
	static std::mt19937 s_RandomEngine;
	static std::uniform_int_distribution<std::mt19937::result_type> s_Distribution;

};
