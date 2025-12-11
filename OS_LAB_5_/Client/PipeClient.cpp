#include "PipeClient.h"
#include <thread>
#include <chrono>
#include <iostream>
#include <windows.h> 

PipeClient::PipeClient(const std::string& pipeName) : pipeName(pipeName), hPipe(INVALID_HANDLE_VALUE) {}
PipeClient::~PipeClient() { close(); }

bool PipeClient::connect() {
    while (true) {
        hPipe = CreateFileA(pipeName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (hPipe != INVALID_HANDLE_VALUE) break;

        DWORD err = GetLastError();
        if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PIPE_BUSY) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            continue;
        }
        else return false;
    }

    DWORD mode = PIPE_READMODE_MESSAGE;
    SetNamedPipeHandleState(hPipe, &mode, NULL, NULL);
    return true;
}

bool PipeClient::sendMessage(const Message& msg) {
    DWORD written;
    BOOL success = WriteFile(hPipe, &msg, sizeof(msg), &written, nullptr) && written == sizeof(msg);
    
    if (success) {
        FlushFileBuffers(hPipe);
    }
    
    if (!success) {
        std::cerr << "Client Error: Failed to send message of type " << msg.type << " (Error: " << GetLastError() << ")\n";
    }
    return success;
}

bool PipeClient::recvMessage(Message& msg) {
    DWORD read;
    BOOL success = ReadFile(hPipe, &msg, sizeof(msg), &read, nullptr) && read == sizeof(msg);

    if (!success) {
        std::cerr << "Client Error: Failed to receive full message (Read: " << read << ", Expected: " << sizeof(msg) << ", Error: " << GetLastError() << ")\n";
    }
    return success;
}

void PipeClient::close() {
    if (hPipe != INVALID_HANDLE_VALUE) {
        ::CloseHandle(hPipe);
        hPipe = INVALID_HANDLE_VALUE;
    }
}