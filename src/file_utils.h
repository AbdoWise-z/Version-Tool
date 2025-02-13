//
// Created by xabdomo on 2/12/25.
//

#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <string>
#include <iostream>
#include <filesystem>
#include <utility>
#include <vector>
#include <memory>

#include "structures.h"

namespace fs = std::filesystem;

struct fTreeNode {
    fs::path path;
    fs::path relativePath;
    bool isDirectory;                // True if the node is a directory
    std::vector<std::shared_ptr<fTreeNode>> children;  // Children nodes

    fTreeNode(fs::path  path, fs::path  relativePath, bool isDirectory)
    : path(std::move(path)), relativePath(std::move(relativePath)), isDirectory(isDirectory), children({}) {}
};

std::string sha256(const char* input, size_t length);
std::string sha256File(const std::string& filename);
bool validateEqual(const fs::path& a, const fs::path& b);
bool validateBlockEqual(const fs::path& a, const fs::path& b, std::streampos offset, size_t blockSize);
void sha256FileBlocks(const std::string& filename, size_t blockSize, std::vector<BlockHash>& blocks);
std::shared_ptr<fTreeNode> buildFileTree(const fs::path& path);
void printTree(const std::shared_ptr<fTreeNode>& node, int level = 0);

#endif //FILE_UTILS_H
