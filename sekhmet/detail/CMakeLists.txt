
include(FetchContent)

# fmt - string formatting
FetchContent_Declare(fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt/
        GIT_TAG 7e4ad40171aa552d38cb99a5c181a0d7b150facc)
FetchContent_MakeAvailable(fmt)
include_directories(${fmt_SOURCE_DIR}/include)

# zstd - (de)compression
FetchContent_Declare(zstd
        GIT_REPOSITORY https://github.com/facebook/zstd/
        GIT_TAG e47e674cd09583ff0503f0f6defd6d23d8b718d3)
FetchContent_GetProperties(zstd)
if(NOT zstd_POPULATED)
    FetchContent_Populate(zstd)

    # Set up & include zstd static library project
    set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "Disable zstd CLI")
    set(ZSTD_BUILD_TESTS OFF CACHE BOOL "Disable zstd tests")
    set(ZSTD_BUILD_SHARED OFF CACHE BOOL "Disable zstd shared target")
    set(ZSTD_MULTITHREAD_SUPPORT ON CACHE BOOL "Enable zstd multithreading")
    set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "Disable zstd CLI")

    add_subdirectory(${zstd_SOURCE_DIR}/build/cmake)
    target_compile_definitions(libzstd_static PRIVATE DYNAMIC_BMI2=1)
    target_compile_definitions(libzstd_static PRIVATE ZSTD_NO_UNUSED_FUNCTIONS)
    target_compile_definitions(libzstd_static PRIVATE ZSTD_MULTITHREAD)

    include_directories(${zstd_SOURCE_DIR}/lib)
endif()
set(zstd libzstd_static)

target_sources(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/uuid.cpp
        ${CMAKE_CURRENT_LIST_DIR}/compound_exception.cpp
        ${CMAKE_CURRENT_LIST_DIR}/filemap.cpp
        ${CMAKE_CURRENT_LIST_DIR}/thread_pool.cpp
        ${CMAKE_CURRENT_LIST_DIR}/intern.cpp
        ${CMAKE_CURRENT_LIST_DIR}/logger.cpp
        ${CMAKE_CURRENT_LIST_DIR}/plugin.cpp
        ${CMAKE_CURRENT_LIST_DIR}/reflection.cpp
        ${CMAKE_CURRENT_LIST_DIR}/zstd_ctx.cpp
        ${CMAKE_CURRENT_LIST_DIR}/asset_stream.cpp
        ${CMAKE_CURRENT_LIST_DIR}/assets.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC fmt zstd)
