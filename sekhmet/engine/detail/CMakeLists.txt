
target_sources(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/logger.cpp
        ${CMAKE_CURRENT_LIST_DIR}/plugin.cpp
        ${CMAKE_CURRENT_LIST_DIR}/zstd_ctx.cpp
        ${CMAKE_CURRENT_LIST_DIR}/assets.cpp
        ${CMAKE_CURRENT_LIST_DIR}/config.cpp)