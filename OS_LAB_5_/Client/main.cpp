#include "ClientApp.h"

int main() {
    ClientApp app(R"(\\.\pipe\EmployeePipe)");
    app.run();
    return 0;
}