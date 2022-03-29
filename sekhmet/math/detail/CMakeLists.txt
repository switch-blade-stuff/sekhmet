
include(${CMAKE_CURRENT_LIST_DIR}/CMake.options.txt)

target_sources(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/sysrandom.cpp)