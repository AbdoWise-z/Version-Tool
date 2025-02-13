//
// Created by xabdomo on 2/12/25.
//

#ifndef ARGS_PARSER_H
#define ARGS_PARSER_H

#include <map>
#include <string>
#include <vector>

struct Option {
    enum Type { BOOL, ENUM, NUMBER, STRING };
    Type type;
    bool required;
    std::vector<std::string> enumValues; // Only used if type == ENUM
    std::string desc; // Description of the option
    std::string defaultValue;
};

std::map<std::string, std::string> parseArgs(int argc, char* argv[], const std::map<std::string, Option>& options);
void printHelp(const std::map<std::string, Option>& options);

#endif //ARGS_PARSER_H
