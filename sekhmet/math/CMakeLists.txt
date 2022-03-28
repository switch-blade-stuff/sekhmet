
target_sources(${PROJECT_NAME} INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/vector.hpp
        ${CMAKE_CURRENT_LIST_DIR}/random.hpp)

include(${CMAKE_CURRENT_LIST_DIR}/detail/CMakeLists.txt)
