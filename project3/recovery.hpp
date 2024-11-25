#ifndef RECOVERY_HPP
#define RECOVERY_HPP

#include <unordered_map>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include "swap_space.hpp"
#include "betree.hpp"

template <class Key, class Value>
class recoveryManager {
    public:
        recoveryManager(swap_space *sspace, betree<Key, Value> *betree_);
        void recoverState();
    private:
        swap_space *sspace;
        betree<Key, Value> *betree_;
        int replayLogs();
        int readMasterLog(std::string& versionMapFilename, std::string& logFilename);
};

#endif