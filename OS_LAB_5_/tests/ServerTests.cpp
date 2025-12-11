#include <gtest/gtest.h>
#include <fstream>
#include <thread>
#include <chrono>
#include <atomic>
#include <windows.h>
#include "Server/RecordManager.h"
#include "Server/ServerApp.h"
#include "common/Employee.h"

TEST(RecordManagerTest, BasicOperations) {
    const std::string testFile = "test_employees.bin";
    
    std::remove(testFile.c_str());
    
    RecordManager manager(testFile);
    
    Employee testEmp;
    testEmp.num = 1;
    strcpy_s(testEmp.name, "John Doe");
    testEmp.hours = 40.5;
    
    manager.records = {testEmp};
    manager.idToIndex[1] = 0;
    manager.recordLocks.push_back(std::make_unique<std::shared_mutex>());
    
    Employee readEmp;
    bool found = manager.readRecordById(1, readEmp);
    EXPECT_TRUE(found);
    
    if (found) {
        EXPECT_EQ(readEmp.num, 1);
        EXPECT_STREQ(readEmp.name, "John Doe");
        EXPECT_EQ(readEmp.hours, 40.5);
    }
    
    Employee notFoundEmp;
    found = manager.readRecordById(999, notFoundEmp);
    EXPECT_FALSE(found);
    
    std::remove(testFile.c_str());
}

TEST(RecordManagerTest, IndexManagementTest) {
    const std::string testFile = "test_index.bin";
    
    RecordManager manager(testFile);
    
    std::vector<Employee> employees = {
        {100, "First", 10.0},
        {200, "Second", 20.0},
        {300, "Third", 30.0}
    };
    
    manager.records = employees;
    for (size_t i = 0; i < employees.size(); i++) {
        manager.idToIndex[employees[i].num] = i;
        manager.recordLocks.push_back(std::make_unique<std::shared_mutex>());
    }
    
    size_t idx;
    
    bool found = manager.getIndexForId(100, idx);
    EXPECT_TRUE(found);
    if (found) EXPECT_EQ(idx, 0);
    
    found = manager.getIndexForId(200, idx);
    EXPECT_TRUE(found);
    if (found) EXPECT_EQ(idx, 1);
    
    found = manager.getIndexForId(999, idx);
    EXPECT_FALSE(found);
    
    std::remove(testFile.c_str());
}

TEST(RecordManagerTest, LockingMechanismTest) {
    const std::string testFile = "test_locks.bin";
    
    RecordManager manager(testFile);
    
    Employee emp{1, "Test", 0.0};
    manager.records = {emp};
    manager.idToIndex[1] = 0;
    manager.recordLocks.push_back(std::make_unique<std::shared_mutex>());
    
    bool lockResult = manager.lockRecord(1, false);
    EXPECT_TRUE(lockResult);
    
    if (lockResult) {
        manager.unlockRecord(1, false);
    }
    
    lockResult = manager.lockRecord(1, true);
    EXPECT_TRUE(lockResult);
    
    if (lockResult) {
        manager.unlockRecord(1, true);
    }
    
    lockResult = manager.lockRecord(999, true);
    EXPECT_FALSE(lockResult);
    
    std::remove(testFile.c_str());
}

TEST(RecordManagerTest, ConstructionTest) {
    const std::string testFile = "test_construct.bin";
    
    {
        RecordManager manager(testFile);
        SUCCEED();
    }
    
    std::remove(testFile.c_str());
}

TEST(RecordManagerTest, SimpleConcurrentAccess) {
    const std::string testFile = "test_concurrent_simple.bin";
    
    RecordManager manager(testFile);
    
    Employee emp{1, "Concurrent", 0.0};
    manager.records = {emp};
    manager.idToIndex[1] = 0;
    manager.recordLocks.push_back(std::make_unique<std::shared_mutex>());
    
    std::atomic<int> successCount{0};
    
    std::vector<std::thread> threads;
    for (int i = 0; i < 3; i++) {
        threads.emplace_back([&, i]() {
            if (manager.lockRecord(1, false)) {
                Employee e;
                if (manager.readRecordById(1, e)) {
                    successCount++;
                }
                manager.unlockRecord(1, false);
            }
        });
    }
    
    threads.emplace_back([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        if (manager.lockRecord(1, true)) {
            Employee updated = emp;
            updated.hours = 50.0;
            if (manager.writeRecord(updated)) {
                successCount++;
            }
            manager.unlockRecord(1, true);
        }
    });
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_GE(successCount, 3);
    
    std::remove(testFile.c_str());
}

TEST(RecordManagerTest, RecordManagerConstructor) {
    RecordManager manager("test.bin");
    EXPECT_EQ(manager.filename, "test.bin");
    EXPECT_TRUE(manager.records.empty());
    EXPECT_TRUE(manager.idToIndex.empty());
    EXPECT_TRUE(manager.recordLocks.empty());
}

TEST(RecordManagerTest, RecordManagerDestructor) {
    {
        RecordManager manager("test.bin");
    }
    SUCCEED();
}

TEST(RecordManagerTest, EmptyManager) {
    RecordManager manager("test.bin");
    
    Employee emp;
    bool found = manager.readRecordById(1, emp);
    EXPECT_FALSE(found);
    
    size_t idx;
    found = manager.getIndexForId(1, idx);
    EXPECT_FALSE(found);
}

TEST(RecordManagerTest, SingleRecord) {
    RecordManager manager("test.bin");
    
    Employee emp{1, "Single", 8.0};
    manager.records = {emp};
    manager.idToIndex[1] = 0;
    manager.recordLocks.push_back(std::make_unique<std::shared_mutex>());
    
    size_t idx;
    bool found = manager.getIndexForId(1, idx);
    EXPECT_TRUE(found);
    EXPECT_EQ(idx, 0);
}

TEST(RecordManagerTest, MultipleRecords) {
    RecordManager manager("test.bin");
    
    std::vector<Employee> employees = {
        {1, "First", 10.0},
        {2, "Second", 20.0},
        {3, "Third", 30.0}
    };
    
    manager.records = employees;
    for (size_t i = 0; i < employees.size(); i++) {
        manager.idToIndex[employees[i].num] = i;
        manager.recordLocks.push_back(std::make_unique<std::shared_mutex>());
    }
    
    EXPECT_EQ(manager.records.size(), 3);
    EXPECT_EQ(manager.idToIndex.size(), 3);
    EXPECT_EQ(manager.recordLocks.size(), 3);
}

TEST(RecordManagerTest, FindNonExistentRecord) {
    RecordManager manager("test.bin");
    
    Employee emp{1, "Test", 5.0};
    manager.records = {emp};
    manager.idToIndex[1] = 0;
    manager.recordLocks.push_back(std::make_unique<std::shared_mutex>());
    
    size_t idx;
    bool found = manager.getIndexForId(999, idx);
    EXPECT_FALSE(found);
}

TEST(RecordManagerTest, IndexConsistency) {
    RecordManager manager("test.bin");
    
    std::vector<Employee> employees = {
        {100, "A", 1.0},
        {200, "B", 2.0},
        {300, "C", 3.0}
    };
    
    manager.records = employees;
    for (size_t i = 0; i < employees.size(); i++) {
        manager.idToIndex[employees[i].num] = i;
        manager.recordLocks.push_back(std::make_unique<std::shared_mutex>());
    }
    
    EXPECT_EQ(manager.idToIndex[100], 0);
    EXPECT_EQ(manager.idToIndex[200], 1);
    EXPECT_EQ(manager.idToIndex[300], 2);
}

TEST(RecordManagerTest, UpdateRecord) {
    RecordManager manager("test.bin");
    
    Employee emp{1, "Old", 10.0};
    manager.records = {emp};
    manager.idToIndex[1] = 0;
    manager.recordLocks.push_back(std::make_unique<std::shared_mutex>());
    
    Employee updated{1, "New", 20.0};
    manager.records[0] = updated;
    
    EXPECT_STREQ(manager.records[0].name, "New");
    EXPECT_EQ(manager.records[0].hours, 20.0);
}

TEST(RecordManagerTest, ReadRecordById) {
    RecordManager manager("test.bin");
    
    Employee emp{42, "Answer", 42.0};
    manager.records = {emp};
    manager.idToIndex[42] = 0;
    manager.recordLocks.push_back(std::make_unique<std::shared_mutex>());
    
    Employee result;
    bool found = manager.readRecordById(42, result);
    
    EXPECT_TRUE(found);
    EXPECT_EQ(result.num, 42);
    EXPECT_STREQ(result.name, "Answer");
    EXPECT_EQ(result.hours, 42.0);
}

TEST(RecordManagerTest, CreateLocks) {
    RecordManager manager("test.bin");
    
    std::vector<Employee> employees(5);
    manager.records = employees;
    
    for (size_t i = 0; i < employees.size(); i++) {
        manager.recordLocks.push_back(std::make_unique<std::shared_mutex>());
    }
    
    EXPECT_EQ(manager.recordLocks.size(), 5);
    for (const auto& lock : manager.recordLocks) {
        EXPECT_NE(lock, nullptr);
    }
}

TEST(RecordManagerTest, EmptyIndex) {
    RecordManager manager("test.bin");
    
    manager.records = std::vector<Employee>(3);
    
    size_t idx;
    bool found = manager.getIndexForId(1, idx);
    EXPECT_FALSE(found);
}

TEST(RecordManagerTest, IndexAfterUpdate) {
    RecordManager manager("test.bin");
    
    std::vector<Employee> employees = {
        {1, "A", 1.0},
        {2, "B", 2.0}
    };
    
    manager.records = employees;
    manager.idToIndex[1] = 0;
    manager.idToIndex[2] = 1;
    
    Employee updated{1, "A Updated", 10.0};
    manager.records[0] = updated;
    
    size_t idx;
    bool found = manager.getIndexForId(1, idx);
    EXPECT_TRUE(found);
    EXPECT_EQ(idx, 0);
}

TEST(RecordManagerTest, EmployeeInRecordManager) {
    RecordManager manager("test.bin");
    
    Employee emp{};
    emp.num = 123;
    strcpy_s(emp.name, "Test Employee");
    emp.hours = 8.5;
    
    manager.records = {emp};
    manager.idToIndex[123] = 0;
    
    EXPECT_EQ(manager.records[0].num, 123);
    EXPECT_STREQ(manager.records[0].name, "Test Employee");
    EXPECT_EQ(manager.records[0].hours, 8.5);
}

TEST(RecordManagerTest, RecordIdentity) {
    RecordManager manager("test.bin");
    
    Employee emp1{1, "Same", 10.0};
    Employee emp2{1, "Same", 10.0};
    
    manager.records = {emp1};
    manager.idToIndex[1] = 0;
    
    EXPECT_EQ(manager.records[0].num, emp2.num);
    EXPECT_STREQ(manager.records[0].name, emp2.name);
    EXPECT_EQ(manager.records[0].hours, emp2.hours);
}

TEST(RecordManagerTest, ContainerSizes) {
    RecordManager manager("test.bin");
    
    std::vector<Employee> employees(10);
    manager.records = employees;
    
    for (size_t i = 0; i < employees.size(); i++) {
        manager.idToIndex[i] = i;
        manager.recordLocks.push_back(std::make_unique<std::shared_mutex>());
    }
    
    EXPECT_EQ(manager.records.size(), 10);
    EXPECT_EQ(manager.idToIndex.size(), 10);
    EXPECT_EQ(manager.recordLocks.size(), 10);
}