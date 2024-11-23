#include "recovery.hpp"

recoveryManager::recoveryManager(swap_space *sspace) : sspace(sspace) {

}

void recoveryManager::recoverState() {
    std::cout << "Recovering State!" << std::endl;
    uint64_t rootId = NULL;
    // Read in Kv_store.log
    // If empty do nothing
    // Else make dictionary and find root
    if(sspace->rebuildVersionMap()) {
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


int recoveryManager::replayLogs() {
    // Read in Log file
    // Check if checkpoint is in file
    // apply logs after checkpoint

}