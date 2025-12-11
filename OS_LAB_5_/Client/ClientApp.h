#pragma once
#include "PipeClient.h"

class ClientApp {
public:
    ClientApp(const std::string& pipeName);
    void run();

public:
    PipeClient client;
    void modifyRecord();
    void readRecord();
};