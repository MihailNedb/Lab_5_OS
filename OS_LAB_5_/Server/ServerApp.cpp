#include "ServerApp.h"
#include <iostream>
#include <thread>
#include <map>
#include <vector>
#include <atomic>
#include <cstring>
#include <limits> 

void SendResponse(HANDLE hPipe, const Message& resp) {
    DWORD bytesTransferred;
    WriteFile(hPipe, &resp, sizeof(Message), &bytesTransferred, nullptr);
    FlushFileBuffers(hPipe); 
}

ServerApp::ServerApp() {
    std::string fname;
    std::cout << "file name: ";
    std::cin >> fname;

    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    
    manager = new RecordManager(fname);
    manager->initRecords();
}

void ServerApp::clientHandler(HANDLE hPipe) {
    Message msg;
    DWORD bytesTransferred = 0;
    std::map<int, bool> heldLocks;

    while (true) {
        BOOL ok = ReadFile(hPipe, &msg, sizeof(msg), &bytesTransferred, nullptr);
        
        if (!ok || bytesTransferred == 0) {
            DWORD err = GetLastError();
            break;
        }

        Message resp;
        std::memset(&resp, 0, sizeof(resp));

        if (msg.type == READ_LOCK) {
            if (manager->lockRecord(msg.id, false)) {
                Employee e;
                bool found = manager->readRecordById(msg.id, e);
                
                if (found) {
                    resp.type = READ_LOCK;
                    resp.id = msg.id;
                    resp.emp = e;
                    
                    if (!WriteFile(hPipe, &resp, sizeof(Message), &bytesTransferred, nullptr)) {
                        manager->unlockRecord(msg.id, false);
                        break;
                    }
                    heldLocks[msg.id] = false;
                } else {
                    resp.type = READ_LOCK;
                    resp.id = -1;
                    WriteFile(hPipe, &resp, sizeof(Message), &bytesTransferred, nullptr);
                    manager->unlockRecord(msg.id, false);
                }
            } else {
                resp.type = READ_LOCK;
                resp.id = -1;
                WriteFile(hPipe, &resp, sizeof(Message), &bytesTransferred, nullptr);
            }
        }
        else if (msg.type == WRITE_LOCK) {
            Employee e;
            bool found = manager->readRecordByIdNoLock(msg.id, e);
            
            if (!found) {
                resp.type = WRITE_LOCK;
                resp.id = -1;
                WriteFile(hPipe, &resp, sizeof(Message), &bytesTransferred, nullptr);
                continue;
            }
            
            if (manager->lockRecord(msg.id, true)) {
                resp.type = WRITE_LOCK;
                resp.id = msg.id;
                resp.emp = e;
                
                if (!WriteFile(hPipe, &resp, sizeof(Message), &bytesTransferred, nullptr)) {
                    manager->unlockRecord(msg.id, true);
                    break;
                }
                heldLocks[msg.id] = true;
            } else {
                resp.type = WRITE_LOCK;
                resp.id = -1;
                WriteFile(hPipe, &resp, sizeof(Message), &bytesTransferred, nullptr);
            }
        }
        else if (msg.type == WRITE_UPDATE) {
            auto lockIt = heldLocks.find(msg.id);
            if (lockIt == heldLocks.end() || !lockIt->second) {
                resp.type = WRITE_UPDATE;
                resp.id = -1;
                WriteFile(hPipe, &resp, sizeof(Message), &bytesTransferred, nullptr);
                continue;
            }
    
            bool success = manager->writeRecord(msg.emp);
            resp.type = WRITE_UPDATE;
            resp.id = success ? msg.id : -1;
            WriteFile(hPipe, &resp, sizeof(Message), &bytesTransferred, nullptr);
        }
        else if (msg.type == UNLOCK) {
            auto it = heldLocks.find(msg.id);
            if (it != heldLocks.end()) {
                manager->unlockRecord(msg.id, it->second);
                heldLocks.erase(it);
            } else {
                manager->unlockRecord(msg.id, true);
                manager->unlockRecord(msg.id, false);
            }
            
            resp.type = UNLOCK;
            resp.id = msg.id;
            WriteFile(hPipe, &resp, sizeof(Message), &bytesTransferred, nullptr);
        }
        else if (msg.type == CLIENT_EXIT) {
            resp.type = CLIENT_EXIT;
            resp.id = 0;
            WriteFile(hPipe, &resp, sizeof(Message), &bytesTransferred, nullptr);
            break;
        }
        else {
            resp.type = 0;
            resp.id = -1;
            WriteFile(hPipe, &resp, sizeof(Message), &bytesTransferred, nullptr);
        }
    }
    
    for (auto& p : heldLocks) {
        try {
            manager->unlockRecord(p.first, p.second);
        } catch (...) {}
    }
    
    FlushFileBuffers(hPipe);
    DisconnectNamedPipe(hPipe);
    CloseHandle(hPipe);
}

void ServerApp::run() {
    int nClients;
    std::cout << "How many clients to run: ";
    std::cin >> nClients;

    std::vector<std::thread> threads;
    std::vector<HANDLE> pipeHandles;

    for (int i = 0; i < nClients; ++i) {
        HANDLE hPipe = CreateNamedPipeA(R"(\\.\pipe\EmployeePipe)",
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            4096, 4096,
            0, NULL);

        if (hPipe == INVALID_HANDLE_VALUE) {
            std::cerr << "error creating named pipe: " << GetLastError() << "\n";
            continue;
        }

        pipeHandles.push_back(hPipe);

        threads.emplace_back([this, hPipe, i]() {
            BOOL connected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
            if (connected) {
                clientHandler(hPipe);
            } else {
                CloseHandle(hPipe);
            }
        });
    }

    std::string clientPath = "Client.exe";
    for (int i = 0; i < nClients; ++i) {
        STARTUPINFOA si{};
        PROCESS_INFORMATION pi{};
        si.cb = sizeof(si);

        std::string cmd = "\"" + clientPath + "\"";

        BOOL ok = CreateProcessA(
            NULL,
            &cmd[0],
            NULL, NULL, FALSE,
            CREATE_NEW_CONSOLE,
            NULL, NULL,
            &si, &pi
        );

        if (!ok) {
            std::cerr << "error running a client " << i << ", error code " << GetLastError() << "\n";
            continue;
        }

        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }

    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }

    std::cout << "Server stopped working.\n";
}