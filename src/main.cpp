#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "core/application/application.h"
#include "utils/logger.h"
#include <iostream>

int main() {
    Core::Application app;
    if (!app.Initialize()) {
        std::cerr << "[main] Initialization failed.\n";
        return 1;
    }
    app.Run();
    return 0;
}
