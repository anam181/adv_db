#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <cstdint>


enum OperationType {
    INSERT,
    UPDATE,
    DELETE
};

struct LogRecord {
    OperationType operation;
    uint64_t key;           
    std::string value;      
    uint64_t timestamp;     

    // how it will be written to txt file - going with json format for readibility but we can change that
	std::string serialize() const {
		return "{\"operation\":" + std::to_string(operation) +
			   ", \"key\":" + std::to_string(key) +
			   ", \"value\":\"" + value +
			   "\", \"timestamp\":" + std::to_string(timestamp) + "}";
	}
};

class Logger {
public:
    Logger(const std::string& filename);
    ~Logger();
    void log(const LogRecord& record);
    void flush();

private:
    std::ofstream log_file;
    std::vector<LogRecord> log_buffer;
    const size_t flush_threshold = 10;
};

#endif
