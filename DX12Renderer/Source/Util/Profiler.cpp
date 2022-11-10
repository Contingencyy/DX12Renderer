#include "Pch.h"
#include "Util/Profiler.h"

#include <imgui/imgui.h>

enum TimerType : uint32_t
{
	CPU = 0,
	GPU,
};

struct ProfilerInternalData
{
	TimerResult FrameTime;
	std::array<std::array<TimerResult, 50>, 2> Timers;
	uint32_t TimerCounts[2] = { 0, 0 };
};

ProfilerInternalData s_Data;

void Profiler::AddFrameTime(const TimerResult& result)
{
	s_Data.FrameTime = result;
}

void Profiler::AddCPUTimer(const TimerResult& result)
{
	s_Data.Timers[CPU][s_Data.TimerCounts[CPU]] = result;
	s_Data.TimerCounts[CPU]++;

	ASSERT(s_Data.TimerCounts[CPU] <= 50, "CPU timers exceeded the maximum allowed amount of 50 timer entries");
}

void Profiler::AddGPUTimer(const TimerResult& result)
{
	s_Data.Timers[GPU][s_Data.TimerCounts[GPU]] = result;
	s_Data.TimerCounts[GPU]++;

	ASSERT(s_Data.TimerCounts[GPU] <= 50, "GPU timers exceeded the maximum allowed amount of 50 timer entries");
}

void Profiler::OnImGuiRender()
{
	ImGui::Begin("Profiler");
	ImGui::Text("Frametime: %.3f ms", s_Data.FrameTime.Duration);
	ImGui::Text("FPS: %u", static_cast<uint32_t>(1000.0f / (s_Data.FrameTime.Duration)));

	std::sort(s_Data.Timers[CPU].begin(), s_Data.Timers[CPU].begin() + s_Data.TimerCounts[CPU], [](TimerResult& lhs, TimerResult& rhs) {
		return lhs.Duration > rhs.Duration;
	});

	if (ImGui::CollapsingHeader("CPU Timers"), ImGuiTreeNodeFlags_DefaultOpen)
	{
		uint32_t currentTimer = 0;

		for (auto& timerResult : s_Data.Timers[CPU])
		{
			char buf[50];
			strcpy_s(buf, timerResult.Name);
			strcat_s(buf, ": %.3fms");

			ImGui::Text(buf, timerResult.Duration);

			currentTimer++;
			if (currentTimer >= s_Data.TimerCounts[CPU])
				break;
		}
	}

	std::sort(s_Data.Timers[GPU].begin(), s_Data.Timers[GPU].begin() + s_Data.TimerCounts[GPU], [](TimerResult& lhs, TimerResult& rhs) {
		return lhs.Duration > rhs.Duration;
	});

	if (ImGui::CollapsingHeader("GPU Timers"), ImGuiTreeNodeFlags_DefaultOpen)
	{
		uint32_t currentTimer = 0;

		for (auto& timerResult : s_Data.Timers[GPU])
		{
			char buf[50];
			strcpy_s(buf, timerResult.Name);
			strcat_s(buf, ": %.3fms");

			ImGui::Text(buf, timerResult.Duration);

			currentTimer++;
			if (currentTimer >= s_Data.TimerCounts[GPU])
				break;
		}
	}

	ImGui::End();
}

void Profiler::Reset()
{
	s_Data.TimerCounts[CPU] = 0;
	s_Data.TimerCounts[GPU] = 0;
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
