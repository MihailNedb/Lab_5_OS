#include "RecordManager.h"
#include <fstream>
#include <iostream>
#include <cstring>

RecordManager::RecordManager(const std::string& filename) : filename(filename) {}

bool RecordManager::getIndexForId(int id, size_t &outIdx) {
    auto it = idToIndex.find(id);
    if (it == idToIndex.end()) return false;
    outIdx = it->second;
    return true;
}

void RecordManager::initRecords() {
    int nRecords;
    std::cout << "Number of records: ";
    std::cin >> nRecords;
    if (nRecords <= 0) return;

    records.clear();
    records.resize(nRecords);

    for (int i = 0; i < nRecords; ++i) {
        Employee e{};
        std::cout << "ID: "; std::cin >> e.num;
        std::string name; std::cout << "name: "; std::cin >> name;
        strncpy_s(e.name, name.c_str(), sizeof(e.name) - 1);
        e.name[sizeof(e.name)-1] = '\0';
        std::cout << "hours: "; std::cin >> e.hours;
        records[i] = e;
    }

    std::ofstream fout(filename, std::ios::binary | std::ios::trunc);
    if (!fout) {
        std::cerr << "Error openning file: " << filename << "\n";
        return;
    }
    for (auto& e : records) {
        fout.write(reinterpret_cast<const char*>(&e), sizeof(e));
        if (!fout) {
            std::cerr << "Error writing in file\n";
            break;
        }
    }
    fout.close();

    idToIndex.clear();
    recordLocks.clear();
    recordLocks.reserve(records.size());
    for (size_t i = 0; i < records.size(); ++i) {
        idToIndex[records[i].num] = i;
        recordLocks.push_back(std::make_unique<std::shared_mutex>());
    }



}

bool RecordManager::readRecordById(int id, Employee& out) {

    
    size_t idx;
    {
        std::lock_guard<std::mutex> lk(indexMutex);
        if (!getIndexForId(id, idx)) {
            std::cout << "RECORD MANAGER: ERROR - ID " << id << " not found in index!\n";
            return false;
        }
    }
 
    out = records[idx];
    return true;
}

bool RecordManager::readRecordByIdNoLock(int id, Employee& out) {
 
    size_t idx;
    {
        std::lock_guard<std::mutex> lk(indexMutex);
        if (!getIndexForId(id, idx)) {
           
            return false;
        }
    }
    
   
    recordLocks[idx]->lock_shared();
    out = records[idx];
    recordLocks[idx]->unlock_shared();
    return true;
}

bool RecordManager::writeRecord(const Employee& e) {

    size_t idx;
    {
        std::lock_guard<std::mutex> lk(indexMutex);
        if (!getIndexForId(e.num, idx)) {
            return false;
        }
    }

    records[idx] = e;

    std::fstream fio(filename, std::ios::in | std::ios::out | std::ios::binary);
    if (!fio) {
        std::cerr << "Error oppening file: " << filename << "\n";
        return false;
    }

    std::streamoff pos = static_cast<std::streamoff>(idx) * static_cast<std::streamoff>(sizeof(Employee));
    fio.seekp(pos, std::ios::beg);
    if (!fio) {
        std::cerr << "seekp failed\n";
        return false;
    }

    fio.write(reinterpret_cast<const char*>(&e), sizeof(e));
    if (!fio) {
        std::cerr << "write failed\n";
        return false;
    }

    fio.flush();
    fio.close();
    return true;
}

bool RecordManager::lockRecord(int id, bool exclusive) {
    size_t idx;
    {
        std::lock_guard<std::mutex> lk(indexMutex);
        if (!getIndexForId(id, idx)) {
            return false;
        }
    }
   
    if (exclusive) {
        recordLocks[idx]->lock();
    } else {
        recordLocks[idx]->lock_shared();
    }
    return true;
}

void RecordManager::unlockRecord(int id, bool exclusive) {
    size_t idx;
    {
        std::lock_guard<std::mutex> lk(indexMutex);
        if (!getIndexForId(id, idx)) {
         
            return;
        }
    }
    
    if (exclusive) {
        recordLocks[idx]->unlock();
    } else {
        recordLocks[idx]->unlock_shared();
    }
}