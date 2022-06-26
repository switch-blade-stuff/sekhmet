
target_sources(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/sysrandom.cpp)

include(${CMAKE_CURRENT_LIST_DIR}/CMake.options.txt)
include(${CMAKE_CURRENT_LIST_DIR}/vector/func/x86/CMakeLists.txt)