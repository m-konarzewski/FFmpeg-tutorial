#include <iostream>

#include "app.h"

int main() {
    try {
        Application application;
        application.Run();
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
    }
}
