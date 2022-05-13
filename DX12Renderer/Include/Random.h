#pragma once

class Random
{
public:
	/* Returns a random float between 0.0f and 1.0f */
	static inline float Float()
	{
		return static_cast<float>(rand() / static_cast<float>(RAND_MAX));
	}

	/* Returns a random float between min and max */
	static inline float FloatRange(float min, float max)
	{
		return min + static_cast<float>(rand() / static_cast<float>(RAND_MAX / (max - min)));
	}
};
