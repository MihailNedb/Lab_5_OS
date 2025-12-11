#pragma once
#include "../common/Employee.h"
#include <string>
#include <windows.h>

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

class PipeClient {
public:
    PipeClient(const std::string& pipeName);
    ~PipeClient();

    bool connect();
    bool sendMessage(const Message& msg);
    bool recvMessage(Message& msg);
    void close();

public:
    std::string pipeName;
    HANDLE hPipe;
};