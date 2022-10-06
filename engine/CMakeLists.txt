
include(${CMAKE_CURRENT_LIST_DIR}/detail/CMakeLists.txt)

target_sources(${PROJECT_NAME} INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/world.hpp)