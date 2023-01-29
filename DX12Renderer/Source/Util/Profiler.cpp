#include "Pch.h"
#include "Util/Profiler.h"

#include <imgui/imgui.h>

TimerResult m_FrameTime;
std::array<std::array<TimerResult, 50>, 2> m_Timers;
uint32_t m_TimerCounts[2] = { 0, 0 };
bool m_CPUTimersOpen = true, m_GPUTimersOpen = true;

enum TimerType : uint32_t
{
	CPU = 0,
	GPU,
};

void Profiler::AddFrameTime(const TimerResult& result)
{
	m_FrameTime = result;
}

void Profiler::AddCPUTimer(const TimerResult& result)
{
	m_Timers[CPU][m_TimerCounts[CPU]] = result;
	m_TimerCounts[CPU]++;

	ASSERT(m_TimerCounts[CPU] <= 50, "CPU timers exceeded the maximum allowed amount of 50 timer entries");
}

void Profiler::AddGPUTimer(const TimerResult& result)
{
	m_Timers[GPU][m_TimerCounts[GPU]] = result;
	m_TimerCounts[GPU]++;

	ASSERT(m_TimerCounts[GPU] <= 50, "GPU timers exceeded the maximum allowed amount of 50 timer entries");
}

void Profiler::OnImGuiRender()
{
	ImGui::Begin("Profiler");
	ImGui::Text("Frametime: %.3f ms", m_FrameTime.Duration);
	ImGui::Text("FPS: %u", static_cast<uint32_t>(1000.0f / (m_FrameTime.Duration)));

	std::sort(m_Timers[CPU].begin(), m_Timers[CPU].begin() + m_TimerCounts[CPU], [](TimerResult& lhs, TimerResult& rhs) {
		return lhs.Duration > rhs.Duration;
	});

	ImGui::SetNextItemOpen(m_CPUTimersOpen);
	if (ImGui::CollapsingHeader("CPU Timers"))
	{
		uint32_t currentTimer = 0;

		for (auto& timerResult : m_Timers[CPU])
		{
			char buf[50];
			strcpy_s(buf, timerResult.Name);
			strcat_s(buf, ": %.3fms");

			ImGui::Text(buf, timerResult.Duration);

			currentTimer++;
			if (currentTimer >= m_TimerCounts[CPU])
				break;
		}
	}
	else if (m_CPUTimersOpen)
	{
		m_CPUTimersOpen = false;
	}

	std::sort(m_Timers[GPU].begin(), m_Timers[GPU].begin() + m_TimerCounts[GPU], [](TimerResult& lhs, TimerResult& rhs) {
		return lhs.Duration > rhs.Duration;
	});

	ImGui::SetNextItemOpen(m_GPUTimersOpen);
	if (ImGui::CollapsingHeader("GPU Timers"))
	{
		uint32_t currentTimer = 0;

		for (auto& timerResult : m_Timers[GPU])
		{
			char buf[50];
			strcpy_s(buf, timerResult.Name);
			strcat_s(buf, ": %.3fms");

			ImGui::Text(buf, timerResult.Duration);

			currentTimer++;
			if (currentTimer >= m_TimerCounts[GPU])
				break;
		}
	}
	else if (m_GPUTimersOpen)
	{
		m_GPUTimersOpen = false;
	}

	ImGui::End();
}

void Profiler::Reset()
{
	m_TimerCounts[CPU] = 0;
	m_TimerCounts[GPU] = 0;
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
	if (strcmp(m_Name, "Frametime") == 0)
		Profiler::AddFrameTime({ m_Name, elapsed.count() * 1000.0f });
	else
		Profiler::AddCPUTimer({ m_Name, elapsed.count() * 1000.0f });
}
