#include "ServerApp.h"
#include <iostream>
#include <limits> 

int main() {
    setlocale(LC_ALL, "rus");
    ServerApp server; 
    server.run();
    return 0;
}