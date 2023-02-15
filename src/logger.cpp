#include <iostream>
#include "logger.hpp"

Logger::Logger(std::string_view filename) {
	file = std::ofstream {filename.data()};
	is_file = true;
}

void Logger::log(std::string_view area, std::string_view text, LogLevel level) {
	std::string str;
	str.reserve(area.size() + text.size() + 2 + 8);
	str += '[';
	str += area;
	str += "]";
	if (level == LogLevel::Info) {
		str += "[info]: ";
	}
	else if (level == LogLevel::Warn) {
		str += "[warn]: ";
	}
	else if (level == LogLevel::Error) {
		str += "[err]: ";
	}
	str += text;
	str += '\n';


	if (is_file) {
		file << str;
	}
	else {
		std::cout << str;
	}
}

Logger::~Logger() {
	if (is_file) {
		file.close();
	}
}
