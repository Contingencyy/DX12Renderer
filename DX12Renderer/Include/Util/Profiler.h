#pragma once
#include <chrono>

struct TimerResult
{
	const char* Name;
	float Duration;
};

class Profiler
{
public:
	static Profiler& Get();

	void AddTimerResult(const TimerResult& result);
	void Reset();

	const std::unordered_map<const char*, TimerResult>& GetTimerResults() const { return m_TimerResults; }

private:
	std::unordered_map<const char*, TimerResult> m_TimerResults;

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
