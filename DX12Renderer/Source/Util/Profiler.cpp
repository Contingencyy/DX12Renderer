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
	s_Data.Timers[TimerType::CPU][s_Data.TimerCounts[TimerType::CPU]] = result;
	s_Data.TimerCounts[TimerType::CPU]++;

	ASSERT(s_Data.TimerCounts[TimerType::CPU] <= 50, "CPU timers exceeded the maximum allowed amount of 50 timer entries");
}

void Profiler::AddGPUTimer(const TimerResult& result)
{
	s_Data.Timers[TimerType::GPU][s_Data.TimerCounts[TimerType::GPU]] = result;
	s_Data.TimerCounts[TimerType::GPU]++;

	ASSERT(s_Data.TimerCounts[TimerType::GPU] <= 50, "GPU timers exceeded the maximum allowed amount of 50 timer entries");
}

void Profiler::OnImGuiRender()
{
	ImGui::Begin("Profiler");
	ImGui::Text("Frametime: %.3f ms", s_Data.FrameTime.Duration);
	ImGui::Text("FPS: %u", static_cast<uint32_t>(1000.0f / (s_Data.FrameTime.Duration)));

	if (ImGui::CollapsingHeader("CPU Timers"))
	{
		uint32_t currentTimer = 0;

		for (auto& timerResult : s_Data.Timers[TimerType::CPU])
		{
			char buf[50];
			strcpy_s(buf, timerResult.Name);
			strcat_s(buf, ": %.3fms");

			ImGui::Text(buf, timerResult.Duration);

			currentTimer++;
			if (currentTimer >= s_Data.TimerCounts[TimerType::CPU])
				break;
		}
	}

	if (ImGui::CollapsingHeader("GPU Timers"))
	{
		uint32_t currentTimer = 0;

		for (auto& timerResult : s_Data.Timers[TimerType::GPU])
		{
			char buf[50];
			strcpy_s(buf, timerResult.Name);
			strcat_s(buf, ": %.3fms");

			ImGui::Text(buf, timerResult.Duration);

			currentTimer++;
			if (currentTimer >= s_Data.TimerCounts[TimerType::GPU])
				break;
		}
	}

	ImGui::End();
}

void Profiler::Reset()
{
	s_Data.TimerCounts[TimerType::CPU] = 0;
	s_Data.TimerCounts[TimerType::GPU] = 0;
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
