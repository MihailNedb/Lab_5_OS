#include "ClientApp.h"
#include <iostream>
#include <string>
#include <cstring>
#include <limits> 

ClientApp::ClientApp(const std::string& pipeName) : client(pipeName) {}

void ClientApp::run() {
    setlocale(LC_ALL, "Russian");
    std::cout << " Client\n";

    if (!client.connect()) {
        std::cerr << "Error connecting to server\n";
        return;
    }

    bool running = true;
    while (running) {
        std::cout << "\n1 - Record modification\n2 - Record reading\n3 - Exit\nChoose: ";
        int choice;
        std::cin >> choice;


        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); 

        switch (choice) {
        case 1:
            modifyRecord();
            break;
        case 2:
            readRecord();
            break;
        case 3:
            client.sendMessage({ CLIENT_EXIT, 0, {} });
            running = false;
            break;
        default:
            std::cout << "Wrong choice\n";
            break;
        }
    }

    client.close();
}

void ClientApp::modifyRecord() {
    int id;
    std::cout << "Enter ID for record modification: ";
    std::cin >> id;
    
  
    std::cin.ignore(1000, '\n'); 


    Message req{ WRITE_LOCK, id };
    if (!client.sendMessage(req)) {
        std::cout << "Error sending lock request\n";
        return;
    }

    Message resp;
    if (!client.recvMessage(resp)) {
        std::cout << "Error receiving lock response\n";
        return;
    }

    if (resp.id == -1) {
        std::cout << "Record wasn't found\n";
        return;
    }

    Employee e = resp.emp;
    std::cout << "Current record: " << e.num << " " << e.name << " " << e.hours << "\n";

    std::string newName;
    double newHours;

    std::cout << "New name: ";
    std::getline(std::cin, newName);

    std::cout << "New hours: ";
    std::cin >> newHours;
    

    while (std::cin.get() != '\n' && !std::cin.eof()) {}

    Employee newE = e;
    strncpy_s(newE.name, newName.c_str(), sizeof(newE.name) - 1);
    newE.name[sizeof(newE.name) - 1] = '\0';
    newE.hours = newHours;

    Message updateMessage{ WRITE_UPDATE, newE.num, newE };

    if (!client.sendMessage(updateMessage)) {
        std::cout << "Error occurred while sending message to the server\n";
        return;
    }

    if (!client.recvMessage(resp)) {
        std::cout << "Error occurred while receiving message from the server\n";
        return;
    }

    if (resp.id == -1) {
        std::cout << "Record update failed\n";
    } else {
        std::cout << "Record changed successfully\n";
    }

    client.sendMessage({ UNLOCK, id });
    
    Message unlockResp;
    if (!client.recvMessage(unlockResp)) {
        std::cout << "Error unlocking record\n";
        return;
    }

    std::cout << "Press Enter to continue...";
    std::cin.get(); 
}

void ClientApp::readRecord() {
    int id;
    std::cout << "Enter ID for reading: ";
    std::cin >> id;
    
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    client.sendMessage({ READ_LOCK, id });
    
    Message resp;
    if (!client.recvMessage(resp)) return;
    
    if (resp.id == -1) {
        std::cout << "Record wasn't found\n";
        return;
    }
    
    Employee e = resp.emp;
    std::cout << "Reading: " << e.num << " " << e.name << " " << e.hours << "\n";
    
    client.sendMessage({ UNLOCK, id });
    
    Message unlockResp;
    if (!client.recvMessage(unlockResp)) {
        std::cout << "Error unlocking record\n";
    }
    
    std::cout << "Press Enter to continue...";
    std::cin.get();
}