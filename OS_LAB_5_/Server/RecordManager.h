#pragma once
#include "../common/Employee.h"
#include <string>
#include <vector>
#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include <mutex>

class RecordManager {
public:
    RecordManager(const std::string& filename);
    void initRecords();
    bool readRecordById(int id, Employee& out);           
    bool readRecordByIdNoLock(int id, Employee& out);   
    bool writeRecord(const Employee& e);
    bool lockRecord(int id, bool exclusive);
    void unlockRecord(int id, bool exclusive);

public:
    std::string filename;
    std::vector<std::unique_ptr<std::shared_mutex>> recordLocks;
    std::unordered_map<int, size_t> idToIndex;
    std::mutex indexMutex;
    std::vector<Employee> records;

    bool getIndexForId(int id, size_t &outIdx);
};