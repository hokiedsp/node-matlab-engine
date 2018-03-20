# Build a shared library named after the project from the files in `src/`
add_library(${PROJECT_NAME} SHARED binding.cpp matlab-engine.cpp matlab-mxarray.cpp)

# Gives our library file a .node extension without any "lib" prefix
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")

# Essential include files to build a node addon,
# You should add this line in every CMake.js based project
target_include_directories(${PROJECT_NAME} PRIVATE ${NodeJS_INCLUDE_DIRS} ${Matlab_INCLUDE_DIRS})

# Essential library files to link to a node addon
# You should add this line in every CMake.js based project
target_link_libraries(${PROJECT_NAME} ${NodeJS_LIBRARIES} ${Matlab_ENG_LIBRARY} ${Matlab_MX_LIBRARY})

if (MSVC)
  target_compile_definitions(${PROJECT_NAME} PRIVATE _SCL_SECURE_NO_WARNINGS)
endif()

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_BINARY_DIR})
