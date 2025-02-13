#include <iostream>
#include <chrono>
#include <thread>

void showProgressWithSpinner(int totalSteps) {
    const char spinner[] = {'|', '/', '-', '\\'};
    int spinnerIndex = 0;

    for (int i = 0; i <= totalSteps; ++i) {
        std::cout << "\r[";

        // Show the progress bar
        int progress = (i * 50) / totalSteps; // 50 is the width of the progress bar
        for (int j = 0; j < progress; ++j) {
            std::cout << "=";
        }
        for (int j = progress; j < 50; ++j) {
            std::cout << " ";
        }

        std::cout << "] " << i << "/" << totalSteps << " ";

        // Show the spinner
        std::cout << spinner[spinnerIndex] << std::flush;

        // Move to the next spinner character
        spinnerIndex = (spinnerIndex + 1) % 4;

        // Simulate work by sleeping for a short period
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << std::endl;
}

int main() {
    std::cout << "Processing... " << std::endl;
    showProgressWithSpinner(100);  // 100 steps for progress
    std::cout << "Done!" << std::endl;

    return 0;
}