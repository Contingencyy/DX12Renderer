#pragma once
#include <chrono>

class Timer
{
public:
	Timer();
	~Timer();

	float Elapsed();

private:
	std::chrono::time_point<std::chrono::steady_clock> m_StartTime;

};
