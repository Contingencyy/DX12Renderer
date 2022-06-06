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
	static void Log(const char* message, Severity severity);

	/* Log a string to console with custom severity level */
	static void Log(const std::string& message, Severity severity);

private:
	static inline const char* SeverityToString(Severity severity);
	static inline void SetSeverityConsoleColor(Severity severity);

};
