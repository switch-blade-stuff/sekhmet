
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

# Add & include RapidJSON
set(RAPIDJSON_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/rapidjson)
set(RAPIDJSON_BUILD_CXX17 ON CACHE BOOL "Enable RapidJSON C++17 support")
add_subdirectory(${RAPIDJSON_SOURCE_DIR} ./rapidjson)
include_directories(${RAPIDJSON_SOURCE_DIR}/include)

# Link all libraries
target_link_libraries(${PROJECT_NAME} PUBLIC fmt libzstd_static)
