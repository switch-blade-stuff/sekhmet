
target_sources(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/uuid.cpp
        ${CMAKE_CURRENT_LIST_DIR}/filemap.cpp
        ${CMAKE_CURRENT_LIST_DIR}/thread_pool.cpp
        ${CMAKE_CURRENT_LIST_DIR}/plugin.cpp
        ${CMAKE_CURRENT_LIST_DIR}/assets.cpp)

include(${CMAKE_CURRENT_LIST_DIR}/adt/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/CMakeLists.options.txt)