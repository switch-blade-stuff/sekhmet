
target_sources(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/logger.cpp
        ${CMAKE_CURRENT_LIST_DIR}/plugin.cpp
        ${CMAKE_CURRENT_LIST_DIR}/zstd.cpp
        ${CMAKE_CURRENT_LIST_DIR}/config.cpp
        ${CMAKE_CURRENT_LIST_DIR}/assets.cpp
        ${CMAKE_CURRENT_LIST_DIR}/resources.cpp
        ${CMAKE_CURRENT_LIST_DIR}/message.cpp
        ${CMAKE_CURRENT_LIST_DIR}/profiler.cpp)
include(${CMAKE_CURRENT_LIST_DIR}/assets/CMakeLists.txt)