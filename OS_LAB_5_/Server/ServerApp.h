#pragma once
#include "RecordManager.h"
#include "../common/Employee.h"
#include <windows.h>
#include <atomic>

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
    ~ServerApp() { delete manager; } // Добавляем деструктор для очистки
    void run();
public:
    RecordManager* manager;
    void clientHandler(HANDLE hPipe);
    
    // Для обеспечения того, что только один поток вызывает initRecords (если бы это было динамически)
    // В текущем коде не используется, но полезно для многопоточных структур
    // std::atomic<bool> stopFlag{false}; 
};