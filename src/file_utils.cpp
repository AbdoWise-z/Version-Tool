//
// Created by xabdomo on 2/12/25.
//

#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include <openssl/evp.h>
#include <cstring>
#include "file_utils.h"


std::string sha256(const char* input, size_t length) {
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to create EVP_MD_CTX");
    }

    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("EVP_DigestInit_ex failed");
    }

    if (EVP_DigestUpdate(ctx, input, length) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("EVP_DigestUpdate failed");
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen;
    if (EVP_DigestFinal_ex(ctx, hash, &hashLen) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("EVP_DigestFinal_ex failed");
    }

    EVP_MD_CTX_free(ctx);

    std::ostringstream hashString;
    for (unsigned int i = 0; i < hashLen; i++) {
        hashString << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return hashString.str();
}

std::string sha256File(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open file: " + filename);
    }

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to create EVP_MD_CTX");
    }

    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("EVP_DigestInit_ex failed");
    }

    std::vector<char> buffer(8192);
    while (file.read(buffer.data(), buffer.size()) || file.gcount()) {
        if (EVP_DigestUpdate(ctx, buffer.data(), file.gcount()) != 1) {
            EVP_MD_CTX_free(ctx);
            throw std::runtime_error("EVP_DigestUpdate failed");
        }
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen;
    if (EVP_DigestFinal_ex(ctx, hash, &hashLen) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("EVP_DigestFinal_ex failed");
    }

    EVP_MD_CTX_free(ctx);

    std::ostringstream hashString;
    for (unsigned int i = 0; i < hashLen; i++) {
        hashString << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return hashString.str();
}

bool validateEqual(const fs::path &a, const fs::path &b) {
    std::ifstream f1(a, std::ios::binary);
    std::ifstream f2(b, std::ios::binary);

    if (!f1 || !f2) {
        std::cerr << "Error opening files.\n";
        return false;
    }

    f1.seekg(0, std::ios::end);
    f2.seekg(0, std::ios::end);
    if (f1.tellg() != f2.tellg()) {
        std::cout << "size didn't match" << std::endl;
        return false;
    }

    f1.seekg(0, std::ios::beg);
    f2.seekg(0, std::ios::beg);

    constexpr size_t BUFFER_SIZE = 1024 * 32;
    char buffer1[BUFFER_SIZE], buffer2[BUFFER_SIZE];

    while (true) {
        f1.read(buffer1, BUFFER_SIZE);
        f2.read(buffer2, BUFFER_SIZE);
        if (std::memcmp(buffer1, buffer2, std::min(BUFFER_SIZE, static_cast<size_t>(f1.gcount()))) != 0) {
            std::cout << "Buffer didn't match" << std::endl;
            return false;
        }

        if (f1.eof() && f2.eof()) {
            return true;
        }

        if (f1.eof() || f2.eof()) {
            return false;
        }
    }
}

bool validateBlockEqual(const fs::path &a, const fs::path &b, std::streampos offset, size_t blockSize) {
    std::ifstream f1(a, std::ios::binary);
    std::ifstream f2(b, std::ios::binary);

    if (!f1 || !f2) {
        std::cerr << "Error opening files.\n";
        return false;
    }

    f1.seekg(offset, std::ios::beg);
    f2.seekg(offset, std::ios::beg);

    auto buffer1 = new char[blockSize];
    auto buffer2 = new char[blockSize];

    f1.read(buffer1, blockSize);
    f2.read(buffer2, blockSize);

    bool areEqual = (f1.gcount() == f2.gcount()) && (std::memcmp(buffer1, buffer2, f1.gcount()) == 0);

    delete[] buffer1;
    delete[] buffer2;

    return areEqual;
}

void sha256FileBlocks(const std::string &filename, size_t blockSize, std::vector<BlockHash> &blocks) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open file: " + filename);
    }

    std::vector<char> buffer(blockSize);
    while (file.read(buffer.data(), static_cast<long>(blockSize)) || file.gcount()) {
        auto readCount = file.gcount();
        while (readCount != blockSize && !file.eof()) {
            file.read(buffer.data() + readCount, static_cast<long>(blockSize) - readCount); // try to fill the entire buffer
        }

        auto hash = sha256(buffer.data(), readCount);
        BlockHash blockHash = {
            .path = filename,
            .index = blocks.size(),
            .hash = hash
        };

        blocks.push_back(blockHash);
    }
}


static std::shared_ptr<fTreeNode> s_buildFileTreeRecv(const fs::path& path, const fs::path& relativePath) {
    if (!fs::exists(path) || !fs::is_directory(path)) {
        return nullptr;
    }

    auto rootNode = std::make_shared<fTreeNode>(path, relativePath, true);
    for (const auto& entry : fs::directory_iterator(path)) {
        if (entry.is_directory()) {
            rootNode->children.push_back(s_buildFileTreeRecv(entry.path(), relativePath / rootNode->path.filename()));
        } else {
            rootNode->children.push_back(std::make_shared<fTreeNode>(entry.path(), relativePath / entry.path().filename(), false));
        }
    }

    return rootNode;
}

std::shared_ptr<fTreeNode> buildFileTree(const fs::path& path) {
    if (!fs::exists(path) || !fs::is_directory(path)) {
        return nullptr;
    }

    auto rootNode = std::make_shared<fTreeNode>(path, "__INPUT_ROOT__", true);

    for (const auto& entry : fs::directory_iterator(path)) {
        if (fs::is_directory(entry)) {
            rootNode->children.push_back(s_buildFileTreeRecv(entry.path(), ""));
        } else {
            rootNode->children.push_back(std::make_shared<fTreeNode>(entry.path(), entry.path().filename(), false));
        }
    }

    return rootNode;
}

// Function to display the tree structure (for debugging or viewing)
void printTree(const std::shared_ptr<fTreeNode>& node, int level) {
    if (node == nullptr) return;

    // Indentation for tree structure view
    std::string indent(level * 2, ' ');

    // Print the current node (directory or file)
    if (node->isDirectory) {
        std::cout << indent << "[DIR] " << node->relativePath.filename() << "\n";
    } else {
        std::cout << indent << "[FILE] " << node->relativePath.filename() << "\n";
    }

    // Recursively print all the children (subdirectories or files)
    for (const auto& child : node->children) {
        printTree(child, level + 1);
    }
}