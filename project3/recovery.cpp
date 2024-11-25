#include "recovery.hpp"

template <class Key, class Value>
recoveryManager<Key, Value>::recoveryManager(swap_space *sspace, betree<Key, Value> *betree_) : sspace(sspace), betree_(betree_) {

}

template <class Key, class Value>
void recoveryManager<Key, Value>::recoverState() {
    std::cout << "Recovering State!" << std::endl;
    uint64_t rootId = NULL;
    std::string versionMapFilename;
    std::string logFilename;

    readMasterLog(versionMapFilename, logFilename);

    // Read in Kv_store.log
    // If empty do nothing
    // Else make dictionary and find root
    if(sspace->rebuildVersionMap(versionMapFilename)) {
        std::cout << "ERROR: rebuilding map" << std::endl;
        return;
    }
    // Rebuild tree using root and dictionary
    if(sspace->root_id != NULL) {
        sspace->rebuildObjectMap();
    }
    // Read log only apply log entries after checkpoint

    replayLogs();
}


template <class Key, class Value>
int recoveryManager<Key, Value>::readMasterLog(std::string& versionMapFilename, std::string& logFilename) {
    std::ifstream file("master.log");
    if (!file.is_open()) {
        std::cerr << "Error: Could not open master log.\n";
        return 1;
    }

    std::string line;
    int linecount = 0;
    while (std::getline(file, line)) {
        linecount++;
        std::size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            if(linecount == 1) {
                logFilename = line.substr(colonPos + 1);
                logFilename.erase(filename.find_last_not_of(" \t") + 1);
            }
            else if(linecount == 2) {
                versionMapFilename = line.substr(colonPos + 1);
                versionMapFilename.erase(filename.find_last_not_of(" \t") + 1);
            }
        }
    }

    file.close();

    return 0;
}


template <class Key, class Value>
int recoveryManager<Key, Value>::replayLogs() {
    // Read in Log file
    // Check if checkpoint is in file
    // apply logs after checkpoint
    betree_->upsert(1, Key k, Value v)
}