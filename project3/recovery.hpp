#ifndef RECOVERY_HPP
#define RECOVERY_HPP

#include <unordered_map>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>

class recoveryManager {
    std::unordered_map<uint64_t, uint64_t> objects_to_versions;
    public:
        recoveryManager();
        void recoverState();
    private:
        int rebuildMap(uint64_t &rootId);
        int rebuildTree(const uint64_t rootId);
        int replayLogs();
};

#endif