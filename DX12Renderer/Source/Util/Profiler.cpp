#include "Pch.h"
#include "Util/Profiler.h"

Profiler& Profiler::Get()
{
	static Profiler s_Instance;
	return s_Instance;
}

void Profiler::AddTimerResult(const TimerResult& result)
{
	if (m_TimerResults.find(result.Name) != m_TimerResults.end())
		m_TimerResults.at(result.Name).Duration += result.Duration;
	else
		m_TimerResults[result.Name] = result;
}

void Profiler::Reset()
{
	m_TimerResults.clear();
}

Timer::Timer(const char* name)
	: m_Name(name), m_IsStopped(false)
{
	m_StartTime = std::chrono::steady_clock::now();
}

Timer::~Timer()
{
	if (!m_IsStopped)
		Stop();
}

void Timer::Stop()
{
	m_IsStopped = true;

	std::chrono::duration<float> elapsed = std::chrono::steady_clock::now() - m_StartTime;
	Profiler::Get().AddTimerResult({ m_Name, elapsed.count() * 1000.0f });
}
