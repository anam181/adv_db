#include "logger.hpp"
#include <iostream>
#include <fstream>

Logger::Logger(const std::string& filename, size_t flush_threshold) 
    : flush_threshold(flush_threshold)  // Initialize flush threshold
{
    log_filename = filename;
    log_file.open(filename, std::ios::app);
    if (!log_file.is_open()) {
        throw std::runtime_error("Unable to open log file: " + filename);
    }
}

Logger::~Logger() {
    flush(); // Ensure all logs are written to file when the logger is destroyed
    log_file.close();
}

void Logger::log(const LogRecord& record) {
    log_buffer.push_back(record);  // Add the log record to the buffer
    if (log_buffer.size() >= flush_threshold) {
        flush();  // Flush to disk if the buffer size exceeds the threshold
    }
}

void Logger::flush() {
    // Write each log record to the file
    for (const auto& record : log_buffer) {
        log_file << record.serialize() << std::endl;
    }
    log_buffer.clear();  // Clear the buffer after flushing
}

std::vector<LogRecord> Logger::get_log_entries() {
    return log_buffer;  
}

std::vector<LogRecord>& Logger::get_log_buffer() {
    return log_buffer;  
}

void Logger::clear_log_on_disk() {
    // Close
    if (log_file.is_open()) {
       log_file.close(); 
    }
    // Empty
    log_file.open(log_filename, std::ofstream::out | std::ofstream::trunc);
}
