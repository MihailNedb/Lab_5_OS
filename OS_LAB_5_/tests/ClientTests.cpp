#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <windows.h>
#include <atomic>
#include "Client/PipeClient.h"
#include "Client/ClientApp.h"

TEST(PipeClientTest, ConnectionTest) {
    HANDLE hPipe = CreateNamedPipeA(
        R"(\\.\pipe\TestEmployeePipe)",
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,
        4096,
        4096,
        0,
        NULL
    );
    
    ASSERT_NE(hPipe, INVALID_HANDLE_VALUE);
    
    std::atomic<bool> serverReady{false};
    std::atomic<bool> connectionMade{false};
    
    std::thread serverThread([&]() {
        serverReady = true;
        ConnectNamedPipe(hPipe, NULL);
        connectionMade = true;
        DisconnectNamedPipe(hPipe);
    });
    
    while (!serverReady) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    PipeClient client(R"(\\.\pipe\TestEmployeePipe)");
    
    bool connected = false;
    auto start = std::chrono::steady_clock::now();
    
    while (std::chrono::steady_clock::now() - start < std::chrono::seconds(1)) {
        connected = client.connect();
        if (connected) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    EXPECT_TRUE(connected);
    
    if (serverThread.joinable()) {
        serverThread.join();
    }
    
    CloseHandle(hPipe);
}

TEST(PipeClientTest, SendReceiveMessageTest) {
    HANDLE hPipe = CreateNamedPipeA(
        R"(\\.\pipe\TestEmployeePipe2)",
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,
        4096,
        4096,
        0,
        NULL
    );
    
    ASSERT_NE(hPipe, INVALID_HANDLE_VALUE);
    
    std::atomic<bool> serverReady{false};
    
    std::thread serverThread([&]() {
        serverReady = true;
        ConnectNamedPipe(hPipe, NULL);
        
        Message receivedMsg;
        DWORD bytesRead;
        BOOL readResult = ReadFile(hPipe, &receivedMsg, sizeof(Message), &bytesRead, NULL);
        
        EXPECT_TRUE(readResult);
        EXPECT_EQ(bytesRead, sizeof(Message));
        EXPECT_EQ(receivedMsg.type, READ_LOCK);
        EXPECT_EQ(receivedMsg.id, 123);
        
        Message responseMsg;
        responseMsg.type = READ_LOCK;
        responseMsg.id = 123;
        responseMsg.emp.num = 123;
        strcpy_s(responseMsg.emp.name, "Test Employee");
        responseMsg.emp.hours = 40.5;
        
        DWORD bytesWritten;
        WriteFile(hPipe, &responseMsg, sizeof(Message), &bytesWritten, NULL);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        DisconnectNamedPipe(hPipe);
    });
    
    while (!serverReady) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    PipeClient client(R"(\\.\pipe\TestEmployeePipe2)");
    
    bool connected = false;
    auto start = std::chrono::steady_clock::now();
    
    while (std::chrono::steady_clock::now() - start < std::chrono::seconds(2)) {
        connected = client.connect();
        if (connected) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    ASSERT_TRUE(connected);
    
    Message sendMsg;
    sendMsg.type = READ_LOCK;
    sendMsg.id = 123;
    EXPECT_TRUE(client.sendMessage(sendMsg));
    
    Message receivedMsg;
    EXPECT_TRUE(client.recvMessage(receivedMsg));
    EXPECT_EQ(receivedMsg.type, READ_LOCK);
    EXPECT_EQ(receivedMsg.id, 123);
    EXPECT_STREQ(receivedMsg.emp.name, "Test Employee");
    EXPECT_EQ(receivedMsg.emp.hours, 40.5);
    
    if (serverThread.joinable()) {
        serverThread.join();
    }
    
    CloseHandle(hPipe);
}

TEST(PipeClientTest, ReconnectionTest) {
    const std::string pipeName = R"(\\.\pipe\ReconnectionTestPipe)";
    
    HANDLE hPipe = CreateNamedPipeA(
        pipeName.c_str(),
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,
        4096,
        4096,
        0,
        NULL
    );
    
    ASSERT_NE(hPipe, INVALID_HANDLE_VALUE);
    
    std::atomic<bool> serverReady{false};
    std::atomic<bool> testCompleted{false};
    
    std::thread serverThread([&]() {
        serverReady = true;
        ConnectNamedPipe(hPipe, NULL);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        DisconnectNamedPipe(hPipe);
        
        ConnectNamedPipe(hPipe, NULL);
        testCompleted = true;
        DisconnectNamedPipe(hPipe);
    });
    
    while (!serverReady) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    PipeClient client(pipeName);
    
    bool connected = false;
    auto start = std::chrono::steady_clock::now();
    
    while (std::chrono::steady_clock::now() - start < std::chrono::seconds(2)) {
        connected = client.connect();
        if (connected) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    EXPECT_TRUE(connected);
    
    client.close();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    bool reconnected = false;
    start = std::chrono::steady_clock::now();
    
    while (std::chrono::steady_clock::now() - start < std::chrono::seconds(2)) {
        reconnected = client.connect();
        if (reconnected) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    EXPECT_TRUE(reconnected);
    
    if (serverThread.joinable()) {
        serverThread.join();
    }
    
    CloseHandle(hPipe);
}

TEST(PipeClientTest, ErrorHandlingTest) {
    PipeClient client(R"(\\.\pipe\NonExistentPipeForTesting)");
    
    HANDLE hPipe = CreateNamedPipeA(
        R"(\\.\pipe\QuickTestPipe)",
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,
        4096,
        4096,
        0,
        NULL
    );
    
    if (hPipe != INVALID_HANDLE_VALUE) {
        CloseHandle(hPipe);
    }
    
    PipeClient client2(R"(\\.\pipe\QuickTestPipe)");
    Message msg;
    msg.type = READ_LOCK;
    msg.id = 1;
}

TEST(PipeClientTest, SimpleCreationTest) {
    {
        PipeClient client(R"(\\.\pipe\TestPipe)");
        SUCCEED();
    }
    
    PipeClient client2(R"(\\.\pipe\TestPipe2)");
    client2.close();
    SUCCEED();
}

TEST(PipeClientTest, MessageStructSize) {
    EXPECT_EQ(sizeof(Message), sizeof(int) * 2 + sizeof(Employee));
    SUCCEED();
}

TEST(PipeClientTest, ZeroInitialization) {
    Message msg = {};
    EXPECT_EQ(msg.type, 0);
    EXPECT_EQ(msg.id, 0);
    EXPECT_EQ(msg.emp.num, 0);
    EXPECT_EQ(msg.emp.name[0], 0);
    EXPECT_EQ(msg.emp.hours, 0.0);
}

TEST(PipeClientTest, CreateEmployee) {
    Employee emp{};
    emp.num = 42;
    strcpy_s(emp.name, "Test Name");
    emp.hours = 8.5;
    
    EXPECT_EQ(emp.num, 42);
    EXPECT_STREQ(emp.name, "Test Name");
    EXPECT_EQ(emp.hours, 8.5);
}

TEST(PipeClientTest, FillNameArray) {
    Employee emp{};
    std::string longName(NAME_SIZE - 1, 'A');
    strcpy_s(emp.name, longName.c_str());
    
    EXPECT_EQ(strlen(emp.name), NAME_SIZE - 1);
    EXPECT_EQ(emp.name[NAME_SIZE - 1], '\0');
}

TEST(PipeClientTest, MsgTypeConstants) {
    EXPECT_NE(READ_LOCK, WRITE_LOCK);
    EXPECT_NE(WRITE_LOCK, WRITE_UPDATE);
    EXPECT_NE(WRITE_UPDATE, UNLOCK);
    EXPECT_NE(UNLOCK, CLIENT_EXIT);
    EXPECT_GT(CLIENT_EXIT, 0);
}

TEST(PipeClientTest, PipeClientConstructor) {
    std::string pipeName = R"(\\.\pipe\testpipe)";
    PipeClient client(pipeName);
    
    EXPECT_EQ(client.pipeName, pipeName);
    EXPECT_EQ(client.hPipe, INVALID_HANDLE_VALUE);
}

TEST(PipeClientTest, PipeClientDestructor) {
    {
        PipeClient client(R"(\\.\pipe\test)");
    }
    SUCCEED();
}

TEST(PipeClientTest, CloseEmptyPipe) {
    PipeClient client(R"(\\.\pipe\test)");
    client.close();
    client.close();
    SUCCEED();
}

TEST(PipeClientTest, SafeStringCopy) {
    Employee emp{};
    std::string name = "Иван Иванов";
    
    strncpy_s(emp.name, name.c_str(), sizeof(emp.name) - 1);
    emp.name[sizeof(emp.name) - 1] = '\0';
    
    EXPECT_STREQ(emp.name, "Иван Иванов");
    EXPECT_EQ(emp.name[sizeof(emp.name) - 1], '\0');
}

TEST(PipeClientTest, CreateMessageWithData) {
    Message msg;
    msg.type = WRITE_UPDATE;
    msg.id = 100;
    
    Employee emp{};
    emp.num = 100;
    strcpy_s(emp.name, "Employee 100");
    emp.hours = 40.0;
    msg.emp = emp;
    
    EXPECT_EQ(msg.type, WRITE_UPDATE);
    EXPECT_EQ(msg.id, 100);
    EXPECT_EQ(msg.emp.num, 100);
    EXPECT_STREQ(msg.emp.name, "Employee 100");
    EXPECT_EQ(msg.emp.hours, 40.0);
}

TEST(PipeClientTest, NameSizeCheck) {
    Employee emp{};
    for (size_t i = 0; i < NAME_SIZE - 1; i++) {
        emp.name[i] = 'A';
    }
    emp.name[NAME_SIZE - 1] = '\0';
    
    EXPECT_EQ(strlen(emp.name), NAME_SIZE - 1);
}

TEST(PipeClientTest, DifferentMessageTypes) {
    Message msgs[5];
    
    msgs[0].type = READ_LOCK;
    msgs[1].type = WRITE_LOCK;
    msgs[2].type = WRITE_UPDATE;
    msgs[3].type = UNLOCK;
    msgs[4].type = CLIENT_EXIT;
    
    for (int i = 0; i < 5; i++) {
        EXPECT_GT(msgs[i].type, 0);
    }
}

TEST(PipeClientTest, EmployeeAssignment) {
    Employee emp1{};
    emp1.num = 1;
    strcpy_s(emp1.name, "First");
    emp1.hours = 10.5;
    
    Employee emp2 = emp1;
    Employee emp3;
    emp3 = emp1;
    
    EXPECT_EQ(emp2.num, 1);
    EXPECT_STREQ(emp2.name, "First");
    EXPECT_EQ(emp2.hours, 10.5);
    
    EXPECT_EQ(emp3.num, 1);
    EXPECT_STREQ(emp3.name, "First");
    EXPECT_EQ(emp3.hours, 10.5);
}

TEST(PipeClientTest, NegativeValues) {
    Employee emp{};
    emp.num = -1;
    emp.hours = -5.5;
    
    EXPECT_EQ(emp.num, -1);
    EXPECT_EQ(emp.hours, -5.5);
}

TEST(PipeClientTest, ZeroValues) {
    Employee emp{};
    EXPECT_EQ(emp.num, 0);
    EXPECT_EQ(emp.name[0], '\0');
    EXPECT_EQ(emp.hours, 0.0);
}