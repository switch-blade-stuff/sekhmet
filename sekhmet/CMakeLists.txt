
target_sources(${PROJECT_NAME} INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/meta.hpp
        ${CMAKE_CURRENT_LIST_DIR}/hash.hpp
        ${CMAKE_CURRENT_LIST_DIR}/utility.hpp
        ${CMAKE_CURRENT_LIST_DIR}/debug.hpp
        ${CMAKE_CURRENT_LIST_DIR}/adapter.hpp
        ${CMAKE_CURRENT_LIST_DIR}/delegate.hpp
        ${CMAKE_CURRENT_LIST_DIR}/event.hpp
        ${CMAKE_CURRENT_LIST_DIR}/array_list.hpp
        ${CMAKE_CURRENT_LIST_DIR}/static_string.hpp
        ${CMAKE_CURRENT_LIST_DIR}/sparse_map.hpp
        ${CMAKE_CURRENT_LIST_DIR}/sparse_set.hpp
        ${CMAKE_CURRENT_LIST_DIR}/dense_map.hpp
        ${CMAKE_CURRENT_LIST_DIR}/dense_set.hpp
        ${CMAKE_CURRENT_LIST_DIR}/mkmap.hpp
        ${CMAKE_CURRENT_LIST_DIR}/intern.hpp
        ${CMAKE_CURRENT_LIST_DIR}/logger.hpp
        ${CMAKE_CURRENT_LIST_DIR}/thread_pool.hpp
        ${CMAKE_CURRENT_LIST_DIR}/type_info.hpp
        ${CMAKE_CURRENT_LIST_DIR}/object.hpp
        ${CMAKE_CURRENT_LIST_DIR}/filemap.hpp
        ${CMAKE_CURRENT_LIST_DIR}/plugin.hpp
        ${CMAKE_CURRENT_LIST_DIR}/math.hpp
        ${CMAKE_CURRENT_LIST_DIR}/assets.hpp)
include(${CMAKE_CURRENT_LIST_DIR}/detail/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/math/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/serialization/CMakeLists.txt)

