#include "Pch.h"
#include "Util/Timer.h"

Timer::Timer()
{
	m_StartTime = std::chrono::steady_clock::now();
}

Timer::~Timer()
{
}

float Timer::Elapsed()
{
	std::chrono::duration<float> elapsed = std::chrono::steady_clock::now() - m_StartTime;
	return elapsed.count();
}
