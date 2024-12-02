#include "logger.hpp"
#include <iostream>
#include <fstream>

uint64_t Logger::curr_lsn = 0;

Logger::Logger(const std::string& filename, uint64_t log_granularity, uint64_t checkpoint_granularity) 
    : log_filename(filename), log_granularity(log_granularity), checkpoint_granularity(checkpoint_granularity)  // Initialize flush threshold
{
    print_log_on_disk();
    std::cout << "Opening log file.." << std::endl;
    log_file.open(log_filename, std::ofstream::out | std::ofstream::app);
    if (!log_file.is_open()) {
        throw std::runtime_error("Unable to open log file: " + filename);
    }
    print_log_on_disk();
}

Logger::~Logger() {
    flush(); // Ensure all logs are written to file when the logger is destroyed
    log_file.close();
}

void Logger::log(const Logger::LogRecord& record) {
    log_buffer.push_back(record);  // Add the log record to the buffer
    if (log_buffer.size() >= log_granularity) {
        flush();  // Flush to disk if the buffer size exceeds the threshold
    }
}

void Logger::flush() {
    // Write each log record to the file
    for (const auto& record : log_buffer) {
        log_file << record.serialize() << std::endl;
    }
    log_file.flush();
    log_buffer.clear();  // Clear the buffer after flushing
}

std::vector<Logger::LogRecord> Logger::get_log_entries() {
    return log_buffer;  
}

std::vector<Logger::LogRecord>& Logger::get_log_buffer() {
    return log_buffer;  
}

void Logger::clear_log_on_disk() {
    std::cout << "Clearing log on disk" << std::endl;
    // Close
    if (log_file.is_open()) {
       log_file.close(); 
    }
    // Empty
    log_file.open(log_filename, std::ofstream::out | std::ofstream::trunc);
}

uint64_t Logger::get_checkpoint_granularity() {
    return checkpoint_granularity;
}

void Logger::print_log_on_disk() {
    std::string line;
    int count = 0;
    std::ifstream file(log_filename);
    std::cout << "Printing log file" << std::endl;
    while (std::getline(file, line)) {
        count++;
        // std::cout << line << std::endl;
    }
    std::cout << count << std::endl;
}
