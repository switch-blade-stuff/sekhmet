
target_sources(${PROJECT_NAME} INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/char_file.hpp
        ${CMAKE_CURRENT_LIST_DIR}/native_file.hpp
        ${CMAKE_CURRENT_LIST_DIR}/native_filemap.hpp)

include(${CMAKE_CURRENT_LIST_DIR}/detail/CMakeLists.txt)