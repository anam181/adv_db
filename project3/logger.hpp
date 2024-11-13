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

class Logger {
public:
    struct LogRecord {
        OperationType operation;
        uint64_t key;           
        std::string value;      
        uint64_t timestamp;     

        // How it will be written to txt file - JSON format for readability
        std::string serialize() const {
            return "{\"lsn\":" + std::to_string(curr_lsn++) + 
                ",\"operation\":" + std::to_string(operation) +
                ", \"key\":" + std::to_string(key) +
                ", \"value\":\"" + value +
                "\", \"timestamp\":" + std::to_string(timestamp) + "}";
        }
    };
    Logger(const std::string& filename, uint64_t log_granularity = 10, uint64_t checkpoint_granularity = 1000);  // Make flush_threshold configurable
    ~Logger();
    
    void log(const Logger::LogRecord& record);
    void flush();  // Flush current buffer to file
    void clear_log_on_disk();  // Clear log buffer after checkpoint
    
    std::vector<Logger::LogRecord>& get_log_buffer(); // Get log buffer for external use
	std::vector<Logger::LogRecord> get_log_entries();
    uint64_t get_checkpoint_granularity();
    void clear_log();                        

private:
    std::string log_filename;
    std::ofstream log_file;
    std::vector<Logger::LogRecord> log_buffer;
    const uint64_t log_granularity;  // Flush threshold, configurable through the constructor
    const uint64_t checkpoint_granularity;
    static u_int64_t curr_lsn;
};

#endif
