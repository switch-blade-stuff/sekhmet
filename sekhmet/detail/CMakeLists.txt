
target_sources(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/plugin.cpp
        ${CMAKE_CURRENT_LIST_DIR}/uuid.cpp
        ${CMAKE_CURRENT_LIST_DIR}/type_info.cpp
        ${CMAKE_CURRENT_LIST_DIR}/thread_pool.cpp)

include(${CMAKE_CURRENT_LIST_DIR}/CMake.options.txt)