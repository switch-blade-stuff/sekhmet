
target_sources(${PROJECT_NAME} INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/logger.hpp
        ${CMAKE_CURRENT_LIST_DIR}/plugin.hpp
        ${CMAKE_CURRENT_LIST_DIR}/assets.hpp
        ${CMAKE_CURRENT_LIST_DIR}/config.hpp
        ${CMAKE_CURRENT_LIST_DIR}/entity.hpp
        ${CMAKE_CURRENT_LIST_DIR}/zstd.hpp)

include(${CMAKE_CURRENT_LIST_DIR}/detail/CMakeLists.txt)