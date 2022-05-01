
target_sources(${PROJECT_NAME} INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/meta.hpp
        ${CMAKE_CURRENT_LIST_DIR}/hash.hpp
        ${CMAKE_CURRENT_LIST_DIR}/utility.hpp
        ${CMAKE_CURRENT_LIST_DIR}/debug.hpp
        ${CMAKE_CURRENT_LIST_DIR}/adapter.hpp
        ${CMAKE_CURRENT_LIST_DIR}/array_list.hpp
        ${CMAKE_CURRENT_LIST_DIR}/static_string.hpp
        ${CMAKE_CURRENT_LIST_DIR}/hmap.hpp
        ${CMAKE_CURRENT_LIST_DIR}/hset.hpp
        ${CMAKE_CURRENT_LIST_DIR}/thread_pool.hpp
        ${CMAKE_CURRENT_LIST_DIR}/type_id.hpp
        ${CMAKE_CURRENT_LIST_DIR}/filemap.hpp
        ${CMAKE_CURRENT_LIST_DIR}/math.hpp
        ${CMAKE_CURRENT_LIST_DIR}/assets.hpp)
include(${CMAKE_CURRENT_LIST_DIR}/detail/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/math/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/serialization/CMakeLists.txt)

