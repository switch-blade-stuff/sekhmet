
include(FetchContent)
FetchContent_Declare(rapidjson GIT_REPOSITORY https://github.com/Tencent/rapidjson/)
set(RAPIDJSON_BUILD_DOC OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(rapidjson)

include_directories(${rapidjson_SOURCE_DIR}/include)

target_sources(${PROJECT_NAME} INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/archive.hpp
        ${CMAKE_CURRENT_LIST_DIR}/ubjson.hpp
        ${CMAKE_CURRENT_LIST_DIR}/json.hpp)