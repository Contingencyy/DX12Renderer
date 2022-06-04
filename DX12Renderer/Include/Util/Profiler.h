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
	static Profiler& Get()
	{
		static Profiler s_Instance;
		return s_Instance;
	}

	void AddTimerResult(const TimerResult& result)
	{
		m_TimerResults.push_back(result);
	}

	const std::vector<TimerResult>& GetTimerResults() const
	{
		return m_TimerResults;
	}

	void Reset()
	{
		m_TimerResults.clear();
	}

private:
	std::vector<TimerResult> m_TimerResults;

};

class Timer
{
public:
	Timer(const char* name)
		: m_Name(name), m_IsStopped(false)
	{
		m_StartTime = std::chrono::steady_clock::now();
	}

	~Timer()
	{
		if (!m_IsStopped)
			Stop();
	}

	void Stop()
	{
		m_IsStopped = true;

		std::chrono::duration<float> elapsed = std::chrono::steady_clock::now() - m_StartTime;
		Profiler::Get().AddTimerResult({ m_Name, elapsed.count() * 1000.0f });
	}

private:
	const char* m_Name;
	std::chrono::time_point<std::chrono::steady_clock> m_StartTime;
	bool m_IsStopped;

};

#define SCOPED_TIMER(name) Timer timer##__LINE__(name)
