
target_sources(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/toml_archive.cpp
        ${CMAKE_CURRENT_LIST_DIR}/ubj_archive.cpp)

# Add UBJF dependency
include(FetchContent)
FetchContent_Declare(ubjf GIT_REPOSITORY https://github.com/switch-blade-stuff/ubj-flow.git)
set(UBJF_NO_TESTS ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(ubjf)