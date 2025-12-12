#pragma once
#include "RecordManager.h"
#include "../common/Employee.h"
#include <windows.h>
#include <atomic>
#include <map>

struct Message {
    int type;
    int id;
    Employee emp;
};

enum MsgType {
    READ_LOCK = 1,
    WRITE_LOCK,
    WRITE_UPDATE,
    UNLOCK,
    CLIENT_EXIT
};

class ServerApp {
public:
    ServerApp();
    ~ServerApp() { delete manager; }
    void run();
public:
    RecordManager* manager;
    void clientHandler(HANDLE hPipe);
    std::map<int, bool> lockedRecords;
  
};