#include <print>

#include "App.hpp"

int main() {
    try {
        App app;
        app.run();
    }
    catch (const std::exception& e) {
        std::println(stderr, "Error: {}", e.what());
    }
    return 0;
}
