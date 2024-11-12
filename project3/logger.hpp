#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <cstdint>

enum OperationType {
    INSERT, // 0
    UPDATE, // 1
    DELETE,  // 2
    CHECKPOINT // 3
};

struct LogRecord {
    OperationType operation;
    uint64_t key;           
    std::string value;      
    uint64_t timestamp;     

    // How it will be written to txt file - JSON format for readability
    std::string serialize() const {
        return "{\"operation\":" + std::to_string(operation) +
               ", \"key\":" + std::to_string(key) +
               ", \"value\":\"" + value +
               "\", \"timestamp\":" + std::to_string(timestamp) + "}";
    }
};

class Logger {
public:
    Logger(const std::string& filename, size_t flush_threshold = 10);  // Make flush_threshold configurable
    ~Logger();
    
    void log(const LogRecord& record);
    void flush();  // Flush current buffer to file
    void clear_log_on_disk();  // Clear log buffer after checkpoint
    
    std::vector<LogRecord>& get_log_buffer(); // Get log buffer for external use
	std::vector<LogRecord> get_log_entries();
    void clear_log();                        

private:
    std::string log_filename;
    std::ofstream log_file;
    std::vector<LogRecord> log_buffer;
    const size_t flush_threshold;  // Flush threshold, configurable through the constructor
};

#endif
