cmake_minimum_required(VERSION 3.3)

# Name of the project (will be the name of the plugin)
project(addon)

list (APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
find_package(NodeJS REQUIRED COMPONENTS NAN)
find_package(Matlab REQUIRED COMPONENTS MX_LIBRARY ENG_LIBRARY)

# Build a shared library named after the project from the files in `src/`
add_library(${PROJECT_NAME} SHARED src/matlab_engine.cpp)

# Gives our library file a .node extension without any "lib" prefix
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")

# Essential include files to build a node addon,
# You should add this line in every CMake.js based project
target_include_directories(${PROJECT_NAME} PRIVATE ${NodeJS_INCLUDE_DIRS} ${Matlab_INCLUDE_DIRS})

# Essential library files to link to a node addon
# You should add this line in every CMake.js based project
target_link_libraries(${PROJECT_NAME} ${NodeJS_LIBRARIES} ${Matlab_MEX_LIBRARY} ${Matlab_ENG_LIBRARY} ${Matlab_MX_LIBRARY})
