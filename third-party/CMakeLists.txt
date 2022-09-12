
# Add & include ZSTD
set(ZSTD_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/zstd)
set(ZSTD_BUILD_SHARED OFF CACHE BOOL "Disable zstd shared target")
set(ZSTD_MULTITHREAD_SUPPORT ON CACHE BOOL "Enable zstd multithreading")

add_subdirectory(${ZSTD_SOURCE_DIR}/build/cmake ./zstd)
target_compile_definitions(libzstd_static PRIVATE DYNAMIC_BMI2=1)
target_compile_definitions(libzstd_static PRIVATE ZSTD_NO_UNUSED_FUNCTIONS)
target_compile_definitions(libzstd_static PRIVATE ZSTD_MULTITHREAD)

include_directories(${ZSTD_SOURCE_DIR}/lib)

# Add & include FMT
set(FMT_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/fmt)
add_subdirectory(${FMT_SOURCE_DIR} ./fmt)
include_directories(${FMT_SOURCE_DIR}/include)

# Add & include ASIO
set(ASIO_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/asio)
set(ASIO_COMPILE_SRC ON CACHE BOOL "Enable ASIO binary compilation")

# ASIO config
add_compile_definitions(ASIO_DISABLE_THREADS)
add_compile_definitions(ASIO_NO_DEPRECATED)
add_compile_definitions(ASIO_NO_TYPEID)
add_compile_definitions(ASIO_DYN_LINK)

# Use URING on linux instead of EPOLL
if (LINUX)
    add_compile_definitions(ASIO_HAS_IO_URING)
    add_compile_definitions(ASIO_HAS_EPOLL)
endif ()

add_subdirectory(${ASIO_SOURCE_DIR} ./asio)
include_directories(${ASIO_SOURCE_DIR}/include)

# Link all libraries
target_link_libraries(${PROJECT_NAME} PUBLIC fmt libzstd_static asio)
