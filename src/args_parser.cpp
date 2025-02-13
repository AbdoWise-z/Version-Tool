//
// Created by xabdomo on 2/12/25.
//

#include "args_parser.h"
#include <sstream>
#include <algorithm>
#include <iostream>

void printHelp(const std::map<std::string, Option>& options) {
    std::cout << "Available options:\n";
    for (const auto& [key, opt] : options) {
        std::cout << "  " << key << " :" << opt.desc << "\n";
    }
}

std::map<std::string, std::string> parseArgs(int argc, char* argv[], const std::map<std::string, Option>& options) {
    std::map<std::string, std::string> parsedArgs;
    for (int i = 1; i < argc; ++i) {

        std::string arg = argv[i];
        if (arg == "-h" || arg == "-help") {
            printHelp(options);
            exit(0);
        }

        if (arg[0] == '-') {
            const std::string& key = arg;
            if (!options.contains(key)) {
                throw std::invalid_argument("Unknown option: " + key);
            }
            if (Option opt = options.at(key); opt.type == Option::BOOL) {
                parsedArgs[key] = "true";
            } else {
                if (i + 1 >= argc) {
                    throw std::invalid_argument("Missing value for option: " + key);
                }
                std::string value = argv[++i];
                if (opt.type == Option::ENUM) {
                    if (std::find(opt.enumValues.begin(), opt.enumValues.end(), value) == opt.enumValues.end()) {
                        throw std::invalid_argument("Invalid value for option: " + key);
                    }
                } else if (opt.type == Option::NUMBER) {
                    std::istringstream iss(value);
                    long double num;
                    if (!(iss >> num)) {
                        throw std::invalid_argument("Invalid number for option: " + key);
                    }
                }
                parsedArgs[key] = value;
            }
        } else {
            throw std::invalid_argument("Unknown option: " + arg);
        }
    }
    for (const auto& [key, opt] : options) {
        if (!parsedArgs.contains(key)) {
            if (opt.required) {
                throw std::invalid_argument("Missing required option: " + key);
            }
            parsedArgs[key] = opt.defaultValue;
        }

    }
    return parsedArgs;
}
