#pragma once

class Logger
{
public:
	enum class Severity
	{
		INFO = 0,
		WARN = (INFO + 1),
		ERR =  (WARN + 1)
	};

public:
	/* Log a string to console with custom severity level */
	static inline void Log(const char* message, Severity severity)
	{
		SetSeverityConsoleColor(severity);

		std::string strMessage(message);
		std::string fullMessage = SeverityToString(severity) + strMessage + "\n";

		printf(fullMessage.c_str());
	}

	/* Log a string to console with custom severity level */
	static inline void Log(const std::string& message, Severity severity)
	{
		SetSeverityConsoleColor(severity);

		std::string fullMessage = SeverityToString(severity) + message + "\n";

		printf(fullMessage.c_str());
	}

private:
	static inline const char* SeverityToString(Severity severity)
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

	static inline void SetSeverityConsoleColor(Severity severity)
	{
		HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

		switch (severity)
		{
		case Severity::INFO:
			SetConsoleTextAttribute(hConsole, 2);
			break;
		case Severity::WARN:
			SetConsoleTextAttribute(hConsole, 6);
			break;
		case Severity::ERR:
			SetConsoleTextAttribute(hConsole, 12);
			break;
		}
	}
};
