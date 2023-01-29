#pragma once
#include <chrono>

struct TimerResult
{
	TimerResult() = default;

	const char* Name = "";
	float Duration = 0.0f;
};

namespace Profiler
{

	void AddFrameTime(const TimerResult& result);
	void AddCPUTimer(const TimerResult& result);
	void AddGPUTimer(const TimerResult& result);
	void OnImGuiRender();
	void Reset();

};

class Timer
{
public:
	Timer(const char* name);
	~Timer();

	void Stop();

private:
	const char* m_Name;
	std::chrono::time_point<std::chrono::steady_clock> m_StartTime;
	bool m_IsStopped;

};

#define SCOPED_TIMER(name) Timer timer##__LINE__(name)
