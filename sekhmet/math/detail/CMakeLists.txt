
#[[ SIMD options ]]
# TODO: Auto detect architecture
option(ENABLE_SIMD "Enable SIMD support for math operations" ON)
if (NOT ENABLE_SIMD)
    add_compile_definitions(SEK_NO_SIMD)
endif ()

target_sources(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/sysrandom.cpp)

include(${CMAKE_CURRENT_LIST_DIR}/vector/func/x86/CMakeLists.txt)