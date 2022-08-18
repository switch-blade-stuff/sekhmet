
target_sources(${PROJECT_NAME} INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/logger.hpp
        ${CMAKE_CURRENT_LIST_DIR}/profiler.hpp
        ${CMAKE_CURRENT_LIST_DIR}/type_info.hpp
        ${CMAKE_CURRENT_LIST_DIR}/type_name.hpp
        ${CMAKE_CURRENT_LIST_DIR}/message.hpp
        ${CMAKE_CURRENT_LIST_DIR}/plugin.hpp
        ${CMAKE_CURRENT_LIST_DIR}/config.hpp
        ${CMAKE_CURRENT_LIST_DIR}/zstd.hpp
        ${CMAKE_CURRENT_LIST_DIR}/assets.hpp
        ${CMAKE_CURRENT_LIST_DIR}/resources.hpp
        ${CMAKE_CURRENT_LIST_DIR}/ecs.hpp)

include(${CMAKE_CURRENT_LIST_DIR}/detail/CMakeLists.txt)