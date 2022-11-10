#pragma once
#include <chrono>

struct TimerResult
{
	TimerResult() = default;

	const char* Name = "";
	float Duration = 0.0f;
};

class Profiler
{
public:
	static void AddFrameTime(const TimerResult& result);
	static void AddCPUTimer(const TimerResult& result);
	static void AddGPUTimer(const TimerResult& result);
	static void OnImGuiRender();
	static void Reset();

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
