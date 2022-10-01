
target_sources(${PROJECT_NAME} INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/base64.hpp
        ${CMAKE_CURRENT_LIST_DIR}/binary.hpp
        ${CMAKE_CURRENT_LIST_DIR}/json.hpp
        ${CMAKE_CURRENT_LIST_DIR}/types.hpp
        ${CMAKE_CURRENT_LIST_DIR}/ubjson.hpp)

include(${CMAKE_CURRENT_LIST_DIR}/detail/CMakeLists.txt)