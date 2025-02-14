cmake_minimum_required(VERSION 3.10)
project(Version-Tool)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(OpenSSL REQUIRED)

include_directories(libs/miniz)

add_executable(vct
        src/main_vct.cpp
        src/args_parser.h
        src/args_parser.cpp
        src/file_utils.h
        src/file_utils.cpp
        libs/miniz/miniz.c
        libs/miniz/miniz.h
        src/structures.h
        src/commands.h
        src/progress_bar.h
        src/progress_bar.cpp
)

add_executable(sandbox
        src/sandbox.cpp
        src/progress_bar.h
        src/progress_bar.cpp
)
target_link_libraries(vct OpenSSL::Crypto)