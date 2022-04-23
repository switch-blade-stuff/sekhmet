
include(FetchContent)

# RapidJSON - runtime json parsing
FetchContent_Declare(rapidjson
        GIT_REPOSITORY  https://github.com/Tencent/rapidjson/
        GIT_TAG         fcb23c2dbf561ec0798529be4f66394d3e4996d8)
FetchContent_GetProperties(rapidjson)
if (NOT rapidjson_POPULATED)
    FetchContent_Populate(rapidjson)
    include_directories(${rapidjson_SOURCE_DIR}/include)
endif ()

target_sources(${PROJECT_NAME} INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/archive.hpp
        ${CMAKE_CURRENT_LIST_DIR}/ubjson.hpp
        ${CMAKE_CURRENT_LIST_DIR}/json.hpp)