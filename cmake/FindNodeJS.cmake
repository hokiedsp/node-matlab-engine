#.rst:
# FindNodeJS
# -------
# Find NodeJS and its native APIs
#
# Optional COMPONENTS are
#    NODE_RUNTIME      << default if no RUNTIME component specified
#    ELECTRON_RUNTIME
#    IOJS_RUNTIME
#    NW_RUNTIME
#    NAN_LIBRARY
#    NAPI_LIBRARY
#
# the Electron include and library
# This module defines
# NLOPT_INCLUDE_DIR, where to find nlopt.h
# NLOPT_LIBRARY, the library to link against to use NLopt.
# NLOPT_SHAREDLIBRARY
# NLOPT_FOUND, If false, do not try to use NLopt.
# NLOPT_ROOT, if this module use this path to find NLopt header
# and libraries.
#
# In Windows, it looks for NLOPT_DIR environment variable if defined


# pkg_check_modules(PC_NodeJS QUIET NodeJS)

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

# define the function to look for a node package (runs npm)
function(FindNodePackage dir ver pkg)
  # check the local node_modules first
  execute_process(COMMAND ${NPM} ${NPM_ARGS} list ${pkg} OUTPUT_VARIABLE list_output)
  string(STRIP ${list_output} list_output)
  string(FIND ${list_output} "\n" pos REVERSE)
  if (pos EQUAL -1)
    message(FATAL_ERROR "expects 2 output lines from \"npm list pkg\"")
  endif()
  math(EXPR pos "${pos}+1")
  string(SUBSTRING ${list_output} ${pos} -1 pkg_info)
  string(FIND ${pkg_info} "@" at_pos)
  if (at_pos EQUAL -1) # not found, try global
    execute_process(COMMAND ${NPM} ${NPM_ARGS} list ${pkg} -g OUTPUT_VARIABLE list_output)
    string(STRIP ${list_output} list_output)
    string(FIND ${list_output} "\n" pos REVERSE)
    if (pos EQUAL -1)
      message(FATAL_ERROR "expects 2 output lines from \"npm list pkg -g\"")
    endif()
    math(EXPR pos "${pos}+1")
    string(SUBSTRING ${list_output} ${pos} -1 pkg_info)
    string(FIND ${pkg_info} "@" at_pos)
  endif()

  if (NOT (at_pos EQUAL -1))
    # retrieve the version string
    math(EXPR at_pos "${at_pos}+1")
    string(SUBSTRING ${pkg_info} ${at_pos} -1 ver_)
    set(${ver} ${ver_} PARENT_SCOPE)
  
    # first line contains the path
    string(FIND ${list_output} " " pos)
    math(EXPR pos "${pos}+1")
    string(FIND ${list_output} "\n" len)
    math(EXPR len ${len}-${pos})
    string(SUBSTRING ${list_output} ${pos} ${len} dir_) # path to the parent of node_modules
    file(TO_CMAKE_PATH ${dir_} dir_)
    string (CONCAT dir_ ${dir_}   "/node_modules/" ${pkg})
    set(${dir} ${dir_} PARENT_SCOPE)
  endif()
endfunction(FindNodePackage)

# sort requested components into runtime & library
foreach(component IN LISTS NodeJS_FIND_COMPONENTS)
  if (component MATCHES .+_RUNTIME)
    list(APPEND runtime_component ${component})
  elseif (component MATCHES .+_LIBRARY)
    message("Found LIBRARY: ${component}")
    list(APPEND library_component ${component})
  else()
    message(FATAL_ERROR "Unknown component: ${component}")
  endif()
endforeach(component IN LISTS NodeJS_FIND_COMPONENTS)

# only 1 RUNTIME component maybe requested
if (DEFINED runtime_component)
  list(LENGTH runtime_component num_runtime)
  if (${num_runtime} GREATER 1)
    message(FATAL_ERROR "Cannot set multiple RUNTIME components.")
  endif()
  list(GET runtime_component 0 runtime_component)
else()
  set(runtime_component "NODE_RUNTIME") # default runtime
endif()
message("Found RUNTIME: ${runtime_component}")

# Create root directory to store runtime headers and .lib files
set(NodeJS_RUNTIME_ROOT_DIR "${CMAKE_BINARY_DIR}" CACHE PATH "Root directory of where to load/save node runtime")
if (NOT IS_DIRECTORY ${NodeJS_RUNTIME_ROOT_DIR})
  message(FATAL_ERROR "NodeJS_RUNTIME_ROOT_DIR is not a valid directory (${NodeJS_RUNTIME_ROOT_DIR}).")
endif()

function (FindNodeJSRuntime root incl_suffix headerurl lib_suffix winliburl)
  # check if the runtime header files have been downloaded already
  find_path (NodeJS_RUNTIME_INCLUDE_DIR node.h
             HINTS ${root}
             PATH_SUFFIXES ${incl_suffix}
             DOC "Include directory of Node runtime library"
             NO_DEFAULT_PATH)

  # if not available, download and unpack the header files
  if (NOT EXISTS NodeJS_RUNTIME_INCLUDE_DIR)
    get_filename_component(DL_DST ${headerurl} NAME)
    set(DL_DST "${CMAKE_BINARY_DIR}/${EL_DST}")
    file(DOWNLOAD ${headerurl}
         ${DL_DST})
    execute_process(COMMAND "cmake" -E tar xzf "${DL_DST}") # unpack root
  endif()

  # find the runtime library file
  find_library (NodeJS_RUNTIME_LIBRARY node
                HINTS ${root}
                PATH_SUFFIXES ${lib_suffix}
                DOC "Node runtime library")

  # if not available additionally download .lib file for windows build
  if (WIN32 AND NOT NodeJS_RUNTIME_LIBRARY)
    set(DL_DST "${root}/lib/node.lib")
    file(DOWNLOAD ${winliburl} ${DL_DST})
    if (EXISTS ${DL_DST})
      set(NodeJS_RUNTIME_LIBRARY "${DL_DST}" CACHE PATH "NodeJS runtime library" FORCE)
    endif ()
  endif ()
endfunction(FindNodeJSRuntime)

if (runtime_component STREQUAL NODE_RUNTIME)
  # Create root directory in build directory (name=packed directory name)
  set(Node_RUNTIME_ROOT_DIR "${NodeJS_RUNTIME_ROOT_DIR}/node-v${NodeJS_Ver}")
  FindNodeJSRuntime(Node_RUNTIME_ROOT_DIR 
                    "include/node" 
                    "https://nodejs.org/dist/v${NodeJS_Ver}/node-v${NodeJS_Ver}-headers.tar.gz" 
                    "lib"
                    "https://nodejs.org/dist/v${NodeJS_Ver}/win-${ARCH_STR}/node.lib")
elseif (runtime_component STREQUAL ELECTRON_RUNTIME)
  # electron package must be installed
  FindNodePackage(NodeJS_Electron_Dir NodeJS_Electron_Ver electron)
  FindNodeJSRuntime(Node_RUNTIME_ROOT_DIR 
                    "include/node" 
                    "https://atom.io/download/atom-shell/v${NodeJS_Electron_Ver}/node-v${NodeJS_Electron_Ver}.tar.gz" 
                    "lib"
                    "https://nodejs.org/dist/v${NodeJS_Ver}/win-${ARCH_STR}/node.lib")
  message("electron package@${NodeJS_Electron_Ver} found at ${NodeJS_Electron_Dir}")
else()
  message(FATAL_ERROR "${runtime_component} is not a valid or supported RUNTIME component.")
endif (runtime_component STREQUAL NODE_RUNTIME)
  
# # https://atom.io/download/atom-shell/v1.8.2/node-v1.8.2.tar.gz
# # https://atom.io/download/atom-shell/v1.8.2/x64/node.lib
# # https://atom.io/download/atom-shell/v1.8.2/node.lib
# endif (NodeJS_RUNTIME EQUAL 0)



# # Check for NAN
# if (NOT EXISTS NodeJS_NAN_FOUND)
#   # check the local modules first
#   FindNodePackage(NodeJS_NAN_Dir NodeJS_NAN_Ver nan)
#   if (NodeJS_NAN_Ver)
#     set(NodeJS_NAN_FOUND ON CACHE BOOL "ON if NodeJS NAN package is installed")
#     set(NodeJS_NAN_INCLUDE_DIR "${NodeJS_NAN_Dir}" CACHE PATH "Include directory for NodeJS NAN package")
#   else()
#     set(NodeJS_NAN_FOUND OFF CACHE BOOL "ON if NodeJS NAN package is installed")
#   endif()
# endif()

# include(FindPackageHandleStandardArgs)
# find_package_handle_standard_args(NodeJS
#   FOUND_VAR NodeJS_FOUND
#   REQUIRED_VARS
#     NodeJS_LIBRARY
#     NodeJS_INCLUDE_DIR
#   VERSION_VAR NodeJS_VERSION
# )

# if(NodeJS_FOUND)
#   set(NodeJS_LIBRARIES ${NodeJS_LIBRARY})
#   set(NodeJS_INCLUDE_DIRS ${NodeJS_INCLUDE_DIR})
#   set(NodeJS_DEFINITIONS ${PC_NodeJS_CFLAGS_OTHER})
# endif()

# mark_as_advanced(
#   NodeJS_INCLUDE_DIR
#   NodeJS_LIBRARY
# )

# # compatibility variables
# set(NodeJS_VERSION_STRING ${NodeJS_VERSION})
