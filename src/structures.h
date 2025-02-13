//
// Created by xabdomo on 2/13/25.
//

#ifndef STRUCTURS_H
#define STRUCTURS_H

#include <filesystem>

namespace fs = std::filesystem;

struct FileHash {
  fs::path path;
  std::string hash;
};

struct BlockHash {
  fs::path path;
  size_t index;
  std::string hash;
};


#endif //STRUCTURS_H
