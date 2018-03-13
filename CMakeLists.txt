cmake_minimum_required(VERSION 2.8)

# Name of the project (will be the name of the plugin)
project(addon)

# file(STRINGS node_modules/nan/package.json NAN_Version REGEX "\"version\": ")
# message("NAN_Version: ${NAN_Version}")

find_program(NodeJS_EXECUTABLE node)
get_filename_component(NodeJS_BIN_DIR ${NodeJS_EXECUTABLE} DIRECTORY)

execute_process(COMMAND "${NodeJS_EXECUTABLE}" --version OUTPUT_VARIABLE NodeJS_Ver)
string(STRIP ${NodeJS_Ver} NodeJS_Ver)
string(SUBSTRING ${NodeJS_Ver} 1 -1 NodeJS_Ver)
message("NodeJS_Ver: ${NodeJS_Ver}")

# Get native target architecture
include(CheckSymbolExists)
if(WIN32)
  check_symbol_exists("_M_AMD64" "" RTC_ARCH_X64)
  if(NOT RTC_ARCH_X64)
    check_symbol_exists("_M_IX86" "" RTC_ARCH_X86)
    if (NOT RTC_ARCH_X86)
      check_symbol_exists("_M_ARM" "" RTC_ARCH_ARM)
    endif()
  endif(NOT RTC_ARCH_X64)
  # add check for arm here
  # see http://msdn.microsoft.com/en-us/library/b0084kay.aspx
else(WIN32)
  check_symbol_exists("__x86_64__" "" RTC_ARCH_X64)
  if(NOT RTC_ARCH_X64)
    check_symbol_exists("__i386__" "" RTC_ARCH_X86)
    if (NOT RTC_ARCH_X86)
      check_symbol_exists("__arm__" "" RTC_ARCH_ARM)
    endif()
  endif(NOT RTC_ARCH_X64)
endif(WIN32)

if(RTC_ARCH_X64)
  set(ARCH_STR x64)
elseif(RTC_ARCH_X86)
  set(ARCH_STR x86)
elseif(RTC_ARCH_ARM)
  set(ARCH_STR ARM)
else()
  message(FATAL_ERROR "Unknown architecture")
endif()

# set command options to run npm
if (WIN32)
  set(NPM "cmd")
  list(APPEND NPM_ARGS "/c" npm)
else(WIN32)
  set(NPM "npm")
  list(LENGTH APPEND)
endif(WIN32)

set(NodeJS_RUNTIME 0) # == NODE_RUNTIME
if (NodeJS_RUNTIME EQUAL 0)
  # Create root directory in build directory (name=packed directory name)
  set(NodeJS_RUNTIME_ROOT_DIR "${CMAKE_BINARY_DIR}/node-v${NodeJS_Ver}")

  # check if the runtime header files have been downloaded already
  find_path (NodeJS_RUNTIME_INCLUDE_DIR node.h
             HINTS ${NodeJS_RUNTIME_ROOT_DIR}
             PATH_SUFFIXES "include/node"
             DOC "Include directory of Node runtime library"
             NO_DEFAULT_PATH)

  # if not available, download and unpack the header files
  if (NOT EXISTS NodeJS_RUNTIME_INCLUDE_DIR)
    set(DL_DST "${CMAKE_BINARY_DIR}/node_headers.tar.gz")
    file(DOWNLOAD
         "https://nodejs.org/dist/v${NodeJS_Ver}/node-v${NodeJS_Ver}-headers.tar.gz"
         ${DL_DST})
    execute_process(COMMAND "cmake" -E tar xzf "${DL_DST}") # unpack NodeJS_RUNTIME_ROOT_DIR
  endif()

  # find the runtime library file
  find_library (NodeJS_RUNTIME_LIBRARY node
                HINTS ${NodeJS_RUNTIME_ROOT_DIR}
                PATH_SUFFIXES "lib"
                DOC "Node runtime library")

  # if not available additionally download .lib file for windows build
  if (WIN32 AND NOT NodeJS_RUNTIME_LIBRARY)
    set(DL_DST "${NodeJS_RUNTIME_ROOT_DIR}/lib/node.lib")
    file(DOWNLOAD 
        "https://nodejs.org/dist/v${NodeJS_Ver}/win-${ARCH_STR}/node.lib"
        ${DL_DST})
    if (EXISTS ${DL_DST})
      set(NodeJS_RUNTIME_LIBRARY "${DL_DST}" CACHE PATH "NodeJS runtime library" FORCE)
    endif ()
  endif ()

  
# https://atom.io/download/atom-shell/v1.8.2/node-v1.8.2.tar.gz
# https://atom.io/download/atom-shell/v1.8.2/x64/node.lib
# https://atom.io/download/atom-shell/v1.8.2/node.lib
endif (NodeJS_RUNTIME EQUAL 0)

# Check for NAN
if (NOT EXISTS NodeJS_NAN_FOUND)
  # check the local modules first
  execute_process(COMMAND ${NPM} ${NPM_ARGS} list nan version OUTPUT_VARIABLE LIST_OUTPUT)

  # LIST_OUTPUT : first line - 
# if ends with (empty)
endif()
# message("NAN_Version: ${NAN_Ver}")

# [ERROR_VARIABLE <variable>]
#                 [INPUT_FILE <file>]
#                 [OUTPUT_FILE <file>]
#                 [ERROR_FILE <file>]
#                 [OUTPUT_QUIET]
#                 [ERROR_QUIET]
#                 [OUTPUT_STRIP_TRAILING_WHITESPACE]
#                 [ERROR_STRIP_TRAILING_WHITESPACE]
#                 [ENCODING <name>])
# find_package(Matlab REQUIRED COMPONENTS MX_LIBRARY ENG_LIBRARY)

# # Build a shared library named after the project from the files in `src/`
# add_library(${PROJECT_NAME} SHARED src/matlab_engine.cpp)

# # Gives our library file a .node extension without any "lib" prefix
# set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")

# # Essential include files to build a node addon,
# # You should add this line in every CMake.js based project
# target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_JS_INC} node_modules/nan ${Matlab_INCLUDE_DIRS})

# # Essential library files to link to a node addon
# # You should add this line in every CMake.js based project
# target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB} ${Matlab_MEX_LIBRARY} ${Matlab_ENG_LIBRARY} ${Matlab_MX_LIBRARY})
