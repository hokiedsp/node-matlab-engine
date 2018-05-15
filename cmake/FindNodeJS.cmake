#.rst:
# FindNodeJS
# -------
# Find Node.js and its native APIs
#
# The module supports the following components:
#
#    NAN              - nan module: Native Abstractions for Node.js 
#    NAPI             - node-addon-api module: N-API
#
# Only 1 Runtime Component (NODE_RUNTIME, ELECTRON_RUNTIME, or NW_RUNTIME) may be specified.
#
# When first configured, a set of files will be downloaded for the specified runtime library. The downloaded
# files are stored in the build directory by default. `NodeJS_RUNTIME_ROOT_DIR` may be specified in order 
# to give the path to store the downloaded runtime.
#
# Module Input Variables
# ^^^^^^^^^^^^^^^^^^^^^^
#
# Users or projects may set the following variables to configure the module
# behaviour:
#
# :NodeJS_RUNTIME_URL     - distribution url (node-gyp:dist-url)
# :NodeJS_RUNTIME_VERSION - runtime version  (node-gyp:target)
# :NodeJS_RUNTIME_NAME    - runtime name (node-gyp:runtime)
#
# Variables defined by the module
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#
# Result variables
# """"""""""""""""
# ``NodeJS_FOUND``
#   ``TRUE`` if the NodeJS installation and runtime files are found, ``FALSE``
#   otherwise. All variable below are defined if NodeJS is found.
# ``NodeJS_EXECUTABLE`` (Cached)
#   Path to the Node.js binary program.
# ``NodeJS_INCLUDE_DIRS`` (Cached)
#  the pathes of the Node.js libraries' headers
# ``NodeJS_LIBRARIES`` (Cached)
#   the whole set of libraries of Node.js
# ``NodeJS_RUNTIME_LIBRARY`` (Cached)
#   library for the selected runtime
# ``NodeJS_RUNTIME_INCLUDE_DIR`` (Cached)
#   include path for the selected runtime headers
# ``NodeJS_UV_INCLUDE_DIR`` (Cached)
#   include path for the libuv library headers (Available only if the component 
#   ``ELECTRON_RUNTIME`` or ``NW_RUNTIME`` is requested)
# ``NodeJS_V8_INCLUDE_DIR`` (Cached)
#   include path for the V8 library headers (Available only if the component 
#   ``ELECTRON_RUNTIME`` or ``NW_RUNTIME`` is requested)
# ``NodeJS_Electron_DIR`` (Cached)
#   path to Node.js electron module (Available only if the component 
#   ``ELECTRON_RUNTIME`` is requested)
# ``NodeJS_Electron_VERSION`` (Cached)
#   installed electron version (Available only if the component 
#   ``ELECTRON_RUNTIME`` is requested)
# ``NodeJS_NW_DIR`` (Cached)
#   path to Node.js webpack module (Available only if the component 
#   ``NW_RUNTIME`` is requested)
# ``NodeJS_NW_VERSION`` (Cached)
#   installed webpack version
# ``NodeJS_NW_LIBRARY`` (Cached)
#   NW library
# ``NodeJS_NAN_FOUND``
#   ``TRUE`` if the NodeJS NAN package is found, ``FALSE``
#   otherwise. Variables NodeJS_NAN_XXX below are defined if NAN is found.
# ``NodeJS_NAN_INCLUDE_DIR`` (Cached)
#   include path for NAN module headers (Available only if the component NAN is requested)
# ``NodeJS_NAN_VERSION`` (Cached)
#   version of NAN module (Available only if the component NAN is requested)
# ``NodeJS_NAPI_FOUND``
#   ``TRUE`` if the NodeJS node-addon-api package is found, ``FALSE``
#   otherwise. All variable NodeJS_NAPI_XXX below are defined if NAPI is found.
# ``NodeJS_NAPI_INCLUDE_DIRS`` (Cached)
#   include path for N-API module headers (Available only if the component NAPI is requested)
# ``NodeJS_NAPI_VERSION`` (Cached)
#   version of node-addon-api module headers (Available only if the component NAPI is requested)
#
# Provided functions
# ^^^^^^^^^^^^^^^^^^
#
# :command:`FindNodePackage`
#   run npm to find an installed package. If success, returns directory and version info.
# :command:`FindNodeJSRuntime`
#   look for runtime files in `NodeJS_RUNTIME_ROOT_DIR`; if not found, download the files.

#node-cmake compatibility
if (NODEJS_URL)
  set(NodeJS_RUNTIME_URL ${NODEJS_URL})
endif()
if (NODEJS_VERSION)
  set(NodeJS_RUNTIME_VERSION ${NODEJS_VERSION})
endif()
if (NODEJS_NAME)
  set(NodeJS_RUNTIME ${NODEJS_NAME})
endif()

# detect NodeJS (which should be on environment path)
if (NOT (NodeJS_EXECUTABLE AND NodeJS_VERSION))
  find_program(NodeJS_EXECUTABLE node)
  get_filename_component(NodeJS_BIN_DIR ${NodeJS_EXECUTABLE} DIRECTORY)

  # parse the version
  execute_process(COMMAND "${NodeJS_EXECUTABLE}" --version OUTPUT_VARIABLE NodeJS_VERSION)
  if (NodeJS_VERSION MATCHES "v([0-9]+[.][0-9]+[.][0-9]+)")
    set(NodeJS_VERSION ${CMAKE_MATCH_1} CACHE INTERNAL "version of Node.js binary")
  endif ()
endif (NOT (NodeJS_EXECUTABLE AND NodeJS_VERSION))

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

# define the function to look for an installed node package (runs npm)
function(FindNodePackage dir ver pkg)
    
  message ("Looking for Node.js package: ${pkg}")

  # search of node_modules for the requested package
  set (basedir_ ${CMAKE_CURRENT_SOURCE_DIR})
  file(GLOB dir_ FOLLOW_SYMLINKS "${basedir_}/node_modules/${pkg}/package.json")
  while (basedir_ AND NOT dir_)
    # go up 2 directories to the parent module
    get_filename_component(basedir_ ${basedir_} DIRECTORY)
    get_filename_component(basedir_ ${basedir_} DIRECTORY)

    # stop at the root module
    if (basedir_ AND IS_DIRECTORY "${basedir_}/node_modules")
      # check parent's node_modules
      file(GLOB dir_ FOLLOW_SYMLINKS "${basedir_}/node_modules/${pkg}/package.json")
    else()
      unset(basedir_)
    endif()
  endwhile()

  if (dir_) # if directory found, look for the version
    # place the directory in the caller's variable
    get_filename_component(dir_ ${dir_} PATH)
    set(${dir} ${dir_} PARENT_SCOPE)
    
    # check version
    execute_process(COMMAND ${NPM} ${NPM_ARGS} list ${pkg} OUTPUT_VARIABLE list_output WORKING_DIRECTORY ${basedir_})
    if(list_output MATCHES "${pkg}@([0-9]+[.][0-9]+[.][0-9]+(rc[0-9]+|-beta[.][0-9]+)?)")
      set(${ver} ${CMAKE_MATCH_1} PARENT_SCOPE)
    endif()
  else()
    unset(${dir} PARENT_SCOPE)
  endif()
  unset(dir_ CACHE)
endfunction(FindNodePackage)

# resolve runtime
list(APPEND electron_urls "https://atom.io/download/electron" 
                          "https://atom.io/download/atom-shell" 
                          "https://gh-contractor-zcbenz.s3.amazonaws.com/atom-shell/dist")
list(APPEND nw_urls "https://dl.nwjs.io/" 
                    "http://node-webkit.s3.amazonaws.com")

if (NOT NodeJS_RUNTIME_URL) # Auto-detect runtime::electron > nw > node
  message("Auto-detecting runtime...")
  FindNodePackage(dir_ ver_ electron)
  if (dir_)
    list(GET electron_urls 0 NodeJS_RUNTIME_URL)
    if (NOT NodeJS_RUNTIME_VERSION)
      set(NodeJS_RUNTIME_VERSION ${ver_})
    endif()
    message("Found electron v${ver_}")
  else()
    FindNodePackage(dir_ ver_ nw)
    if (dir_)
      list(GET nw_urls 0 NodeJS_RUNTIME_URL)
      if (NOT NodeJS_RUNTIME_VERSION)
        set(NodeJS_RUNTIME_VERSION ${ver_})
      endif()
      message("Found nw v${ver_}")
    else() # no special runtime specified, use node.js runtime
      set(NodeJS_RUNTIME_URL "https://nodejs.org/dist")
      if (NOT NodeJS_RUNTIME_VERSION)
        set(NodeJS_RUNTIME_VERSION ${NodeJS_VERSION})
      endif()
      message("No external runtime found. Use node v${NodeJS_VERSION}")
    endif()
  endif()
elseif (NOT NodeJS_RUNTIME_VERSION) # Auto-detect version
  if (NodeJS_RUNTIME_URL IN_LIST electron_urls)
    FindNodePackage(dir_ NodeJS_RUNTIME_VERSION electron)
    if (NOT dir_)
      message(FATAL_ERROR "NodeJS_RUNTIME_URL is set to ${NodeJS_RUNTIME_URL}, but electron is not found in node_modules to retrieve NodeJS_RUNTIME_VERSION")
    endif()
  elseif(NodeJS_RUNTIME_URL IN_LIST nw_urls)
    FindNodePackage(dir_ NodeJS_RUNTIME_VERSION nw)
    if (NOT dir_)
      message(FATAL_ERROR "NodeJS_RUNTIME_URL is set to ${NodeJS_RUNTIME_URL}, but nw is not found in node_modules to retrieve NodeJS_RUNTIME_VERSION")
    endif()
  else()
    set(NodeJS_RUNTIME_VERSION ${NodeJS_VERSION})
  endif()
endif()

if (NOT NodeJS_RUNTIME)
  if (NodeJS_RUNTIME_URL IN_LIST electron_urls)
    set(NodeJS_RUNTIME "electron")
  elseif (NodeJS_RUNTIME_URL IN_LIST nw_urls)
    set(NodeJS_RUNTIME "nw")
  else()
    set(NodeJS_RUNTIME "node")
  endif()
endif()

list(APPEND header_archive_hints "node-v${NodeJS_RUNTIME_VERSION}-headers.tar.gz" #node
                                 "node-v${NodeJS_RUNTIME_VERSION}.tar.gz"         #electron
                                 "${NodeJS_RUNTIME}-headers-v${ver_}.tar.gz")     #nw
list(APPEND node_h_path_suffix "node-v${NodeJS_RUNTIME_VERSION}/include/node"      #node
                              "node-v${NodeJS_RUNTIME_VERSION}/src"              #electron
                              "node/src")                                         #nw
list(APPEND v8_h_path_suffix "node-v${NodeJS_RUNTIME_VERSION}/include/node"        #node
                            "node-v${NodeJS_RUNTIME_VERSION}/deps/v8/include"      #electron
                            "node/deps/v8/include")                                #nw
list(APPEND uv_h_path_suffix "node-v${NodeJS_RUNTIME_VERSION}/include/node"        #node
                            "node-v${NodeJS_RUNTIME_VERSION}/deps/uv/include"      #electron
                            "node/deps/uv/include")                                #nw
if (WIN32)
  if (RTC_ARCH_X64)
    list(APPEND lib_hints "win-x64/node.lib"
                          "x64/node.lib"
                          "x64/${NodeJS_RUNTIME}.lib"
                          "win-x64/${NodeJS_RUNTIME}.lib")
  else()
    list(APPEND lib_hints "win-x86/node.lib"
                          "node.lib"
                          "${NodeJS_RUNTIME}.lib"
                          "win-x86/${NodeJS_RUNTIME}.lib")
  endif()
endif()
set(NodeJS_RUNTIME_ROOT_DIR "${CMAKE_BINARY_DIR}/${NodeJS_RUNTIME}")
set(NodeJS_RUNTIME_ROOT_URL "${NodeJS_RUNTIME_URL}/v${NodeJS_RUNTIME_VERSION}")

# Check for existing include #
find_path (NodeJS_RUNTIME_INCLUDE_DIR node.h
           PATHS ${NodeJS_RUNTIME_ROOT_DIR}
           PATH_SUFFIXES ${node_h_path_suffix}
           DOC "Include directory of Node runtime library"
           NO_DEFAULT_PATH)
if (WIN32)
  find_library (NodeJS_RUNTIME_LIBRARY NAMES node ${NodeJS_RUNTIME} 
                HINTS "${NodeJS_RUNTIME_ROOT_DIR}" 
                DOC "Node runtime library")
endif()

# if files are missing, attempt to download
if (NOT EXISTS ${NodeJS_RUNTIME_INCLUDE_DIR} OR (WIN32 AND NOT EXISTS ${NodeJS_RUNTIME_LIBRARY}))

  function (download srcs filepath)
    set(found false)
    foreach(src ${srcs})
      get_filename_component(dst ${src} NAME)
      set(dst ${NodeJS_RUNTIME_ROOT_DIR}/${dst})

      file(DOWNLOAD "${NodeJS_RUNTIME_ROOT_URL}/${src}" ${dst} INACTIVITY_TIMEOUT 10 STATUS DL_STATUS)
      list(GET DL_STATUS 0 status)
      if (NOT status EQUAL 0) # failed to download, go to next choice
        file(REMOVE ${dst})
        continue()
      endif()

      # message("URL: ${NodeJS_RUNTIME_ROOT_URL}/${src}")
      # message("Save to: ${dst}")

      # if hash file downloaded, check
      if (NodeJS_RUNTIME_CHECKSUM_FOUND)
        # determine the hash (empty if not found)
        string(REPLACE "." "[.]" regex_expr ${src})
        string(CONCAT regex_expr "([A-Fa-f0-9]+)[ \t]+" ${regex_expr})
        if (NodeJS_RUNTIME_CHECKSUM MATCHES ${regex_expr})
          set(checksum ${CMAKE_MATCH_1})
          if(checksum)
            file(SHA256 ${dst} actual_checksum)
            if (checksum STREQUAL actual_checksum)
              set(found true)
              break()
            else() # checksum failed
              message(FATAL_ERROR "Checksum failed\n\texpected: ${checksum}\nactual: ${actual_checksum}")
            endif()
          else() # no checksum given, good to go
            set(found true)
            break()
          endif()
        else() # no checksum file found, good to go
          set(found true)
        endif()
      endif()
    endforeach()
    if (found)
      set(${filepath} ${dst} PARENT_SCOPE)
    else()
      list(GET DL_STATUS 1 msg)
      message(FATAL_ERROR "File not found on ${NodeJS_RUNTIME_ROOT_URL}\n\t${_msg}")
    endif()
  endfunction()

  # fresh start
  file(REMOVE_RECURSE ${NodeJS_RUNTIME_ROOT_DIR})

  # download checksum hash file
  set(NodeJS_RUNTIME_CHECKSUM ${NodeJS_RUNTIME_ROOT_DIR}/SHASUMS256.txt)
  file(DOWNLOAD
    ${NodeJS_RUNTIME_ROOT_URL}/SHASUMS256.txt
    ${NodeJS_RUNTIME_CHECKSUM}
    INACTIVITY_TIMEOUT 10
    STATUS DL_STATUS)
  list(GET DL_STATUS 0 DL_STATUS)
  if (DL_STATUS EQUAL 0)
    set(NodeJS_RUNTIME_CHECKSUM_FOUND true)
    file(READ ${NodeJS_RUNTIME_CHECKSUM} NodeJS_RUNTIME_CHECKSUM)
  else()
    set(NodeJS_RUNTIME_CHECKSUM_FOUND false)
  endif()
  
  # download header
  download("${header_archive_hints}" NodeJS_RUNTIME_HEADER_ARCHIVE)
  
  # unpack headers
  execute_process(COMMAND "cmake" -E tar xzf 
                                  "${NodeJS_RUNTIME_HEADER_ARCHIVE}" 
                                  WORKING_DIRECTORY "${NodeJS_RUNTIME_ROOT_DIR}") 
    
  # download windows runtime .lib
  if (WIN32)
    download("${lib_hints}" NodeJS_RUNTIME_LIBRARY)
    message("NodeJS_RUNTIME_LIBRARY: ${NodeJS_RUNTIME_LIBRARY}")
    set (NodeJS_RUNTIME_LIBRARY ${NodeJS_RUNTIME_LIBRARY} CACHE FILEPATH "Node runtime library")
  endif()

  # Recheck 
  find_path (NodeJS_RUNTIME_INCLUDE_DIR node.h
              PATHS ${NodeJS_RUNTIME_ROOT_DIR}
              PATH_SUFFIXES ${node_h_path_suffix}
              DOC "Include directory of Node runtime library"
              NO_DEFAULT_PATH)
endif()

# find additional include dirs
find_path (NodeJS_V8_INCLUDE_DIR v8.h
          PATHS ${NodeJS_RUNTIME_ROOT_DIR}
          PATH_SUFFIXES ${v8_h_path_suffix}
          DOC "Include directory of NodeJS v8 headers"
          NO_DEFAULT_PATH)
find_path (NodeJS_UV_INCLUDE_DIR uv.h
          PATHS ${NodeJS_RUNTIME_ROOT_DIR}
          PATH_SUFFIXES ${uv_h_path_suffix}
          DOC "Include directory of NodeJS uv headers"
          NO_DEFAULT_PATH)

# Check for NAN
if (NAN IN_LIST NodeJS_FIND_COMPONENTS)
  if (NOT NodeJS_NAN_INCLUDE_DIR)
    # check the local modules first
    FindNodePackage(dir_ ver_ nan)
    set(NodeJS_NAN_INCLUDE_DIR ${dir_} CACHE PATH "NAN Node Module Directory")
    set(NodeJS_NAN_VERSION ${ver_} CACHE INTERNAL "NAN Node Module Version")
  endif()
  if (NodeJS_NAN_INCLUDE_DIR)
    set(NodeJS_NAN_FOUND TRUE)
  else()
    set(NodeJS_NAN_FOUND FALSE)
  endif()
endif (NAN IN_LIST NodeJS_FIND_COMPONENTS)

# Check for NAPI
if (NAPI IN_LIST NodeJS_FIND_COMPONENTS)
  if (NOT NodeJS_NAPI_INCLUDE_DIRS)
    # check the local modules first
    FindNodePackage(dir_ ver_ "node-addon-api")

    # only support Node.js newer than v8.5
    if (NodeJS_VERSION VERSION_LESS_EQUAL "8.5.0")
      message(FATAL_ERROR "Node.js version 8.5.0 or earlier is not supported.")
    endif (NodeJS_VERSION VERSION_LESS_EQUAL "8.5.0")

    set(NodeJS_NAPI_VERSION ${ver_} CACHE INTERNAL "N-API Node Module Version")
    if (dir_)
      # need to include 2 directories
      list(APPEND dir_ "${dir_}/external-napi")
      set(NodeJS_NAPI_INCLUDE_DIRS ${dir_} CACHE PATH "N-API Node Module Directory")
    endif (dir_)
  endif(NOT NodeJS_NAPI_INCLUDE_DIRS)

  if (NodeJS_NAPI_INCLUDE_DIRS)
    set(NodeJS_NAPI_FOUND TRUE)
  else()
    set(NodeJS_NAPI_FOUND FALSE)
  endif()
endif (NAPI IN_LIST NodeJS_FIND_COMPONENTS)

set(req_vars NodeJS_RUNTIME_INCLUDE_DIR)
if (WIN32)
  list(APPEND req_vars NodeJS_RUNTIME_LIBRARY)
endif()

# check all the required components are set
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(NodeJS
  FOUND_VAR NodeJS_FOUND
  REQUIRED_VARS ${req_vars}
  HANDLE_COMPONENTS
  VERSION_VAR NodeJS_RUNTIME_VERSION
)

if(NodeJS_FOUND)
  list(APPEND NodeJS_INCLUDE_DIRS ${NodeJS_RUNTIME_INCLUDE_DIR} ${NodeJS_V8_INCLUDE_DIR} ${NodeJS_UV_INCLUDE_DIR}
                                  ${NodeJS_NAN_INCLUDE_DIR} ${NodeJS_NAPI_INCLUDE_DIRS})
  list(REMOVE_DUPLICATES NodeJS_INCLUDE_DIRS)
  list(APPEND NodeJS_LIBRARIES ${NodeJS_RUNTIME_LIBRARY})

  if(NOT WIN32)
    # Non-windows platforms should use these flags
    list(APPEND NodeJS_DEFINITIONS _LARGEFILE_SOURCE _FILE_OFFSET_BITS=64)

    # Special handling for OSX / clang to allow undefined symbols
    # Define is required by node on OSX
    if(APPLE)
      list(APPEND NodeJS_DEFINITIONS _DARWIN_USE_64_BIT_INODE=1)
    endif()
  endif()
endif()

mark_as_advanced(
  NodeJS_EXECUTABLE
  NodeJS_VERSION
  NodeJS_RUNTIME_DIR
  NodeJS_RUNTIME_VERSION
  NodeJS_NAN_INCLUDE_DIR
  NodeJS_NAN_VERSION
  NodeJS_NAPI_INCLUDE_DIRS
  NodeJS_NAPI_VERSION
  NodeJS_RUNTIME_INCLUDE_DIR
  NodeJS_RUNTIME_LIBRARY
  NodeJS_UV_INCLUDE_DIR
  NodeJS_V8_INCLUDE_DIR
)

# compatibility variables
set(NodeJS_VERSION_STRING ${NodeJS_VERSION})
