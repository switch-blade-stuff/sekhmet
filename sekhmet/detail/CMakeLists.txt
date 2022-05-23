
include(FetchContent)

# fmt - string formatting
FetchContent_Declare(fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt/
        GIT_TAG 7e4ad40171aa552d38cb99a5c181a0d7b150facc)
FetchContent_MakeAvailable(fmt)
include_directories(${fmt_SOURCE_DIR}/include)

target_sources(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/uuid.cpp
        ${CMAKE_CURRENT_LIST_DIR}/filemap.cpp
        ${CMAKE_CURRENT_LIST_DIR}/thread_pool.cpp
        ${CMAKE_CURRENT_LIST_DIR}/intern.cpp
        ${CMAKE_CURRENT_LIST_DIR}/logger.cpp
        ${CMAKE_CURRENT_LIST_DIR}/plugin.cpp
        ${CMAKE_CURRENT_LIST_DIR}/reflection.cpp
        ${CMAKE_CURRENT_LIST_DIR}/assets.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC fmt)
