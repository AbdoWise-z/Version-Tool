#pragma once

#include <iostream>
#include <cstdlib>
#include <filesystem>
#include <chrono>
#include <sstream>
#include <random>

inline std::filesystem::path getCacheDirectory() {
    const char* tempDir = nullptr;

#if defined(_WIN32) || defined(_WIN64)
    // Windows-specific temporary directory
    tempDir = std::getenv("TEMP");
    if (tempDir == nullptr) {
        tempDir = std::getenv("TMP");
    }
#else
    // UNIX-like systems (Linux/macOS) - check for TMPDIR environment variable
    tempDir = std::getenv("TMPDIR");
    if (tempDir == nullptr) {
        tempDir = "/tmp";  // Default fallback for Linux/macOS
    }
#endif

    if (tempDir == nullptr) {
        std::cerr << "Error: No temporary directory available." << std::endl;
        exit(1);
    }

    // Return the path to the temporary directory
    return std::filesystem::path(tempDir);
}


inline std::filesystem::path getUniqueTempDir() {
    // Get the system's temporary directory
    const char* tempDir = nullptr;

#if defined(_WIN32) || defined(_WIN64)
    // Windows-specific temporary directory
    tempDir = std::getenv("TEMP");
    if (tempDir == nullptr) {
        tempDir = std::getenv("TMP");
    }
#else
    // UNIX-like systems (Linux/macOS)
    tempDir = std::getenv("TMPDIR");
    if (tempDir == nullptr) {
        tempDir = "/tmp";  // Default fallback for Linux/macOS
    }
#endif

    if (tempDir == nullptr) {
        std::cerr << "Error: No temporary directory available." << std::endl;
        exit(1);
    }

    auto now = std::chrono::system_clock::now();
    auto timeSinceEpoch = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

    // Generate a random string to further ensure uniqueness
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15); // Hexadecimal characters

    std::stringstream randStringStream;
    randStringStream << std::hex;
    for (int i = 0; i < 8; ++i) {
        randStringStream << dis(gen);
    }

    std::string dirName = "vt_" + std::to_string(timeSinceEpoch) + "_" + randStringStream.str();
    std::filesystem::path uniqueTempDir = std::filesystem::path(tempDir) / dirName;

    if (std::filesystem::create_directory(uniqueTempDir)) {
        std::cout << "Created unique temporary directory: " << uniqueTempDir << std::endl;
    } else {
        std::cerr << "Failed to create temporary directory." << std::endl;
        exit(1);
    }

    return uniqueTempDir;
}
