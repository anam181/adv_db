#include "logger.hpp"
#include <iostream>

Logger::Logger(const std::string& filename) {
    log_file.open(filename, std::ios::app);
    if (!log_file.is_open()) {
        throw std::runtime_error("Unable to open log file: " + filename);
    }
}

Logger::~Logger() {
    flush();
    log_file.close();
}

void Logger::log(const LogRecord& record) {
    log_buffer.push_back(record);
    if (log_buffer.size() >= flush_threshold) {
        flush();
    }
}

void Logger::flush() {
    for (const auto& record : log_buffer) {
        log_file << record.serialize() << std::endl;
    }
    log_buffer.clear();
}
