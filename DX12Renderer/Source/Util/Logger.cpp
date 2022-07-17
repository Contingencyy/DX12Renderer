#include "Pch.h"
#include "Util/Logger.h"

void Logger::Log(const char* message, Severity severity)
{
	SetSeverityConsoleColor(severity);

	std::string strMessage(message);
	std::string fullMessage = SeverityToString(severity) + strMessage + "\n";

	printf(fullMessage.c_str());
}

void Logger::Log(const std::string& message, Severity severity)
{
	SetSeverityConsoleColor(severity);

	std::string fullMessage = SeverityToString(severity) + message + "\n";

	printf(fullMessage.c_str());
}

const char* Logger::SeverityToString(Severity severity)
{
	switch (severity)
	{
	case Severity::INFO:
		return "[INFO] ";
		break;
	case Severity::WARN:
		return "[WARN] ";
		break;
	case Severity::ERR:
		return "[ERR] ";
		break;
	}
}

inline void Logger::SetSeverityConsoleColor(Severity severity)
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	switch (severity)
	{
	case Severity::INFO:
		SetConsoleTextAttribute(hConsole, 7);
		break;
	case Severity::WARN:
		SetConsoleTextAttribute(hConsole, 6);
		break;
	case Severity::ERR:
		SetConsoleTextAttribute(hConsole, 12);
		break;
	}
}
