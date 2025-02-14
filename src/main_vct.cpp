//
// Created by xabdomo on 2/12/25.
//

#include <iostream>
#include "args_parser.h"
#include <fstream>
#include <vector>
#include <sstream>
#include <cstring>
#include "miniz.h"
#include "env.hpp"
#include "file_utils.h"
#include "structures.h"
#include "commands.h"
#include "progress_bar.h"

bool addFileToZip(mz_zip_archive &zip, const fs::path &filePath, const fs::path &basePath) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Failed to open file: " << filePath << "\n";
        return false;
    }

    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> buffer(fileSize);
    if (!file.read(buffer.data(), fileSize)) {
        std::cerr << "Failed to read file: " << filePath << "\n";
        return false;
    }

    std::string zipPath = fs::relative(filePath, basePath).string();

    if (!mz_zip_writer_add_mem(&zip, zipPath.c_str(), buffer.data(), buffer.size(), MZ_BEST_COMPRESSION)) {
        std::cerr << "Failed to add file to ZIP: " << zipPath << "\n";
        return false;
    }

    return true;
}

// Function to zip an entire folder
bool zipFolder(const std::string &folderPath, const std::string &zipFilePath) {
    mz_zip_archive zip = {};

    if (!mz_zip_writer_init_file(&zip, zipFilePath.c_str(), 0)) {
        std::cerr << "Failed to create ZIP file: " << zipFilePath << "\n";
        return false;
    }

    fs::path basePath(folderPath);
    for (const auto &entry: fs::recursive_directory_iterator(basePath)) {
        if (fs::is_regular_file(entry.path())) {
            if (!addFileToZip(zip, entry.path(), basePath)) {
                mz_zip_writer_end(&zip);
                return false;
            }
        }
    }

    mz_zip_writer_finalize_archive(&zip);
    mz_zip_writer_end(&zip);

    return true;
}


static void bfsListFiles(const fTreeNode *root, std::vector<std::pair<fs::path, fs::path> > &paths) {
    for (const auto &it: root->children) {
        paths.push_back({it->path, it->relativePath});
    }

    for (const auto &it: root->children) {
        if (!it->children.empty()) {
            bfsListFiles(it.get(), paths);
        }
    }
}

template<typename T>
void writeToBuffer(const T &obj, char *buffer) {
    std::memcpy(buffer, &obj, sizeof(T));
}

// Deserialize: Read an object of type T from a buffer
template<typename T>
T readFromBuffer(const char *buffer) {
    T obj;
    std::memcpy(&obj, buffer, sizeof(T));
    return obj;
}

int main(int argc, char *argv[]) {
    std::map<std::string, Option> options;
    options["-from"] = {
        .type = Option::STRING,
        .required = true,
        .enumValues = {},
        .desc = "a path to the root of the folder that contains the version you're updating from (required)",
        .defaultValue = "",
    };

    options["-to"] = {
        .type = Option::STRING,
        .required = true,
        .enumValues = {},
        .desc = "a path to the root of the folder that contains the version you're updating to (required)",
        .defaultValue = "",
    };

    options["-vm"] = {
        .type = Option::ENUM,
        .required = false,
        .enumValues = {"input", "output", "all", "none"},
        .desc = "which validation files should be created ? (not required)"
        "\n     \"input\"  -> create validation files to validate the input only."
        "\n     \"output\" -> create validation files to validate the output only."
        "\n     \"all\"    -> create validation files to validate both input & output."
        "\n     \"none\"   -> don't create validation files.",
        .defaultValue = "all",
    };

    options["-o"] = {
        .type = Option::STRING,
        .required = false,
        .enumValues = {},
        .desc = "where to write output (not required)",
        .defaultValue = "./v-diff.zip",
    };

    options["-bs"] = {
        .type = Option::NUMBER,
        .required = false,
        .enumValues = {},
        .desc = "the size of each block",
        .defaultValue = "8192", // 8 KB
    };

    auto args = parseArgs(argc, argv, options);

    const std::string src_path = args["-from"];
    const std::string dst_path = args["-to"];
    const std::string vm = args["-vm"];
    const std::string output = args["-o"];
    std::istringstream blockSizeStream(args["-bs"]);
    size_t blockSize;
    blockSizeStream >> blockSize;

    std::filesystem::path cacheDir = getUniqueTempDir();
    std::cout << "Cache directory: " << cacheDir << std::endl;

    printf("Creating v-diff file for \"%s\" -> \"%s\", \nUsing validation: %s\nOutput: %s\n", src_path.c_str(),
           dst_path.c_str(), vm.c_str(), output.c_str());

    // build input tree

    std::cout << "Listing inputs .. ";
    auto input_tree = buildFileTree(src_path);
    std::vector<std::pair<fs::path, fs::path> > input_files;
    bfsListFiles(input_tree.get(), input_files); // list all files (using bfs) and set a file index as it's id
    std::cout << "Done" << std::endl;

    // prepare inputs hashes
    std::cout << "Prepare Input Hashes .. ";
    std::map<size_t, FileHash> inputFilesHashes;
    std::map<size_t, std::vector<BlockHash> > inputFilesBlocksHashes;
    for (const auto& i : progress_bar::ranged<long>(0, input_files.size() - 1, 1, "Prepare Input Hashes")) {
        const auto &path = input_files[i];
        inputFilesHashes[i] = {
            .path = path.second,
            .hash = sha256File(path.first),
        };

        std::vector<BlockHash> fileBlocksHashes;
        sha256FileBlocks(path.first, blockSize, fileBlocksHashes);
        inputFilesBlocksHashes[i] = fileBlocksHashes;
    }
    std::cout << " .. Done" << std::endl;


    // write hashes in a file for validation
    if (vm == "all" || vm == "input") {
        std::cout << "Input validation is enabled. building input validation file." << std::endl;
        auto iv = cacheDir / "iv";
        std::ofstream iv_file(iv);
        for (const auto &it: progress_bar::from(inputFilesHashes, inputFilesHashes.size() - 1, "Input Hashes")) {
            iv_file << it.second.path << " " << it.second.hash << std::endl;
        }
        iv_file.close();
        std::cout << " .. Done" << std::endl;
    }

    // write input list ids
    std::ofstream input_listing_file(cacheDir / "input_list");
    for (const auto &path: input_files) {
        input_listing_file << path.first << " " << path.second << std::endl;
    }
    input_listing_file.close();

    // build the output files tree
    std::cout << "Listing outputs .. ";
    auto output_tree = buildFileTree(dst_path);
    std::vector<std::pair<fs::path, fs::path> > output_files;
    bfsListFiles(output_tree.get(), output_files); // list all files (using bfs) and set a file index as it's id
    std::cout << "Done" << std::endl;

    // list all outputs hashes
    std::cout << "Prepare Output Hashes .. ";
    std::map<size_t, FileHash> outputFilesHashes;
    std::map<size_t, std::vector<BlockHash> > outputFilesBlocksHashes;
    for (int i = 0; i < output_files.size(); ++i) {
        const auto &path = output_files[i];
        outputFilesHashes[i] = {
            .path = path.second,
            .hash = sha256File(path.first),
        };

        std::vector<BlockHash> fileBlocksHashes;
        sha256FileBlocks(path.first, blockSize, fileBlocksHashes);
        outputFilesBlocksHashes[i] = fileBlocksHashes;
    }
    std::cout << "Done" << std::endl;

    // created inverted hash map to search for output hashes inside the inputs quickly
    std::cout << "Prepare Inverted Index .. ";
    std::map<fs::path, size_t> invertedInputList; // each (relative) path can only have one file so it's ok
    std::map<std::string, std::vector<size_t> > invertedFilesHashes;
    // more than one file can have the same hash .. its hard to happen .. but possible
    std::map<std::string, std::vector<std::pair<size_t, size_t> > > invertedBlocksHashes;
    // more than one block can have the same hash
    for (int i = 0; i < input_files.size(); ++i) {
        const auto &path = input_files[i];
        invertedInputList[path.second] = i;
        invertedFilesHashes[inputFilesHashes[i].hash].emplace_back(i);
        for (const auto &it: inputFilesBlocksHashes[i]) {
            invertedBlocksHashes[it.hash].emplace_back(static_cast<size_t>(i), it.index);
            //direct the hash to the index-th block in the i-th file
        }
    }
    std::cout << "Done" << std::endl;

    std::cout << "Writing Update Files .. " << std::endl;
    const auto rootDir = cacheDir / "data";
    fs::create_directories(rootDir);
    constexpr size_t WRITER_BUFFER_SIZE = 64 * 1024;
    auto writer_buffer = new char[WRITER_BUFFER_SIZE];
    for (int i = 0; i < output_files.size(); ++i) {
        std::ofstream file_writer(rootDir / output_files[i].second);
        std::ifstream file_reader(output_files[i].first, std::ios::binary);
        const auto &path = output_files[i];
        const auto &hash = outputFilesHashes[i];
        const auto &blockHashes = outputFilesBlocksHashes[i];

        // option 1: try to find a file with the exact hash and check if it actually equal to this file .. if so then just copy it
        const auto &matchingFiles = invertedFilesHashes[hash.hash];
        bool write_complete = false;
        for (const auto &it: matchingFiles) {
            if (validateEqual(input_files[it].first, path.first)) {
                // these two files are the exact same :) ... good news we only need to reference this
                // input file in the update file
                writeToBuffer(COPY_FILE, writer_buffer);
                writeToBuffer(it, writer_buffer + sizeof(char));
                writeToBuffer(DONE, writer_buffer + sizeof(char) + sizeof(size_t));
                file_writer.write(writer_buffer, sizeof(char) * 2 + sizeof(size_t));
                write_complete = true;
                break;
            } else {
                std::cout << "   | same hash but diff files: " << input_files[it].first << " " << path.first <<
                        std::endl;
            }
        }

        if (write_complete) {
            file_writer.close();
            file_reader.close();
            continue;
        }

        // option 2: go block by block .. and try to match each block
        for (const auto &blockHash: blockHashes) {
            const auto &matchingBlocks = invertedBlocksHashes[blockHash.hash];
            bool block_write_complete = false;
            for (const auto &it: matchingBlocks) {
                const auto &matchedFilePath = input_files[it.first].first;
                if (validateBlockEqual(matchedFilePath, path.first, it.second * blockSize, blockSize)) {
                    // block is indeed equal .. write command to copy it
                    writeToBuffer(COPY_BLOCK, writer_buffer);
                    writeToBuffer(it.first, writer_buffer + sizeof(char));
                    writeToBuffer(it.second, writer_buffer + sizeof(char) + sizeof(size_t));
                    // no done here .. done indicates the end of a file
                    file_writer.write(writer_buffer, sizeof(char) + sizeof(size_t) * 2);
                    block_write_complete = true;
                    break;
                }
            }

            if (block_write_complete) {
                continue;
            }

            // was unable to find any block from the input that can be copied to the output .. then just dumb the entire thing
            writeToBuffer(WRITE_BLOCK, writer_buffer);
            file_writer.write(writer_buffer, sizeof(char));
            size_t r = blockSize;
            file_reader.seekg(blockHash.index * blockSize, std::ios::beg);
            while (r > 0 && !file_reader.eof()) {
                file_reader.read(writer_buffer, std::min<size_t>(r, WRITER_BUFFER_SIZE));
                file_writer.write(writer_buffer, file_reader.gcount());
                r -= file_reader.gcount();
            }
        }

        writeToBuffer(DONE, writer_buffer);
        file_writer.write(writer_buffer, sizeof(char));
        file_reader.close();
        file_writer.close();

        std::cout << " >> " << path.first << std::endl;
    }

    delete[] writer_buffer;
    std::cout << "Done" << std::endl;

    // write hashes in a file for validation
    if (vm == "all" || vm == "output") {
        std::cout << "Output validation is enabled. building output validation file .. ";
        auto ov = cacheDir / "ov";
        std::ofstream ov_file(ov);
        for (const auto &it: outputFilesHashes) {
            ov_file << it.second.path << " " << it.second.hash << std::endl;
        }
        ov_file.close();
        std::cout << "Done" << std::endl;
    }

    std::cout << "Zipping Files .. ";
    if (zipFolder(cacheDir, output)) {
        std::cout << "Done" << std::endl;
    } else {
        std::cout << "Failed (see errors)" << std::endl;
    }

    return 0;
}
