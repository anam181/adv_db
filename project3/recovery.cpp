#include "recovery.hpp"

recoveryManager::recoveryManager() {

}

void recoveryManager::recoverState() {
    std::cout << "Recovering State!" << std::endl;
    uint64_t rootId = NULL;
    // Read in Kv_store.log
    // If empty do nothing
    // Else make dictionary and find root
    if(rebuildMap(rootId)) {
        std::cout << "ERROR: rebuilding map" << std::endl;
        return;
    }
    // Rebuild tree using root and dictionary
    if(rootId != NULL) {
        rebuildTree(rootId);
    }
    // Read log only apply log entries after checkpoint
    replayLogs();
}

int recoveryManager::rebuildMap(uint64_t &rootId) {
    // Read in file and parse out the data
    std::ifstream file("kv_store.log");
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file.\n";
        return 1;
    }

    std::string line;
    int lineCount = 0;

    while (std::getline(file, line)) {
        if(lineCount == 0) {
            rootId = std::stoull(line);
        }
        else {
            size_t pos = line.find(':');

            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);

                objects_to_versions[std::stoull(key)] = std::stoull(value);
            } else {
                std::cerr << "Delimiter ':' not found!" << std::endl;
                return 1;
            }
        }
    }

    file.close();
    return 0;
}

int recoveryManager::rebuildTree(const uint64_t rootId) {
    // Read in node from file
    // Input into tree
    // Loop through children and DFS to add to tree
}

int recoveryManager::replayLogs() {
    // Read in Log file
    // Check if checkpoint is in file
    // apply logs after checkpoint

}