
target_sources(${PROJECT_NAME} INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/archive.hpp
        ${CMAKE_CURRENT_LIST_DIR}/binary.hpp
        ${CMAKE_CURRENT_LIST_DIR}/ubjson.hpp
        ${CMAKE_CURRENT_LIST_DIR}/json.hpp)

include(${CMAKE_CURRENT_LIST_DIR}/detail/CMakeLists.txt)