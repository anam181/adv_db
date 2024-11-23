#ifndef RECOVERY_HPP
#define RECOVERY_HPP

#include <unordered_map>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include "swap_space.hpp"

class recoveryManager {
    std::unordered_map<uint64_t, uint64_t> objects_to_versions;
    public:
        recoveryManager(swap_space *sspace);
        void recoverState();
    private:
        swap_space *sspace;
        int replayLogs();
};

#endif