
target_sources(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/delegate_error.cpp
        ${CMAKE_CURRENT_LIST_DIR}/expected.cpp
        ${CMAKE_CURRENT_LIST_DIR}/intern.cpp
        ${CMAKE_CURRENT_LIST_DIR}/object.cpp
        ${CMAKE_CURRENT_LIST_DIR}/uuid.cpp
        ${CMAKE_CURRENT_LIST_DIR}/uri.cpp
        ${CMAKE_CURRENT_LIST_DIR}/thread_pool.cpp)

include(${CMAKE_CURRENT_LIST_DIR}/type_info/CMakeLists.txt)
