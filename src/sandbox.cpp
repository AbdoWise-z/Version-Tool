#include <iostream>
#include <chrono>
#include <thread>

#include "progress_bar.h"

int main() {

    for (const auto& i : progress_bar::ranged<double>(100 , 0, -0.5, "Sandbox")) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return 0;
}