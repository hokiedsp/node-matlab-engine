cmake_minimum_required(VERSION 3.3)

# Name of the project (will be the name of the plugin)
project(addon)

set(CMAKE_EXPORT_COMPILE_COMMANDS true)

list (APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
find_package(NodeJS REQUIRED COMPONENTS)
find_package(Matlab REQUIRED COMPONENTS MX_LIBRARY ENG_LIBRARY)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory("src")
