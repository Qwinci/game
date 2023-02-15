#pragma once
#include <string_view>
#include <fstream>

enum class LogLevel {
	Info,
	Warn,
	Error
};

class Logger {
public:
	Logger() : is_file {false} {}
	~Logger();
	explicit Logger(std::string_view filename);
	void log(std::string_view area, std::string_view text, LogLevel level = LogLevel::Info);
private:
	bool is_file;
	union {
		std::ofstream file;
	};
};