
#[[ SIMD options ]]
# TODO: Auto detect architecture
option(ENABLE_SIMD "Enable SIMD support for math operations" ON)
if (NOT ENABLE_SIMD)
    add_compile_definitions(SEK_NO_SIMD)
endif ()

if (ENABLE_SIMD)
    option(ENABLE_SSE "Enable SSE instruction set support" ON)
    if (ENABLE_SSE)
        add_compile_definitions(SEK_USE_SSE)
        option(ENABLE_SSE2 "Enable SSE2 instruction set support" ON)
        if (ENABLE_SSE2)
            add_compile_definitions(SEK_USE_SSE2)
        endif ()
        option(ENABLE_SSE3 "Enable SSE3 instruction set support" ON)
        if (ENABLE_SSE3)
            add_compile_definitions(SEK_USE_SSE3)
        endif ()
        option(ENABLE_SSSE3 "Enable SSSE3 instruction set support" ON)
        if (ENABLE_SSSE3)
            add_compile_definitions(SEK_USE_SSSE3)
        endif ()
        option(ENABLE_SSE4 "Enable SSE4 instruction set support" ON)
        if (ENABLE_SSE4)
            add_compile_definitions(SEK_USE_SSE4)
            option(ENABLE_SSE4_1 "Enable SSE4.1 instruction set support" ON)
            if (ENABLE_SSE4_1)
                add_compile_definitions(SEK_USE_SSE4_1)
            endif ()
            option(ENABLE_SSE4_2 "Enable SSE4.2 instruction set support" ON)
            if (ENABLE_SSE4_2)
                add_compile_definitions(SEK_USE_SSE4_2)
            endif ()
        endif ()
    endif ()

    option(ENABLE_AVX "Enable AVX instruction set support" OFF)
    if (ENABLE_AVX)
        add_compile_definitions(SEK_USE_AVX)
        option(ENABLE_AVX2 "Enable AVX2 instruction set support" OFF)
        if (ENABLE_AVX2)
            add_compile_definitions(SEK_USE_AVX2)
        endif ()
    endif ()
endif ()

target_sources(${PROJECT_NAME} INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/utility.hpp
        ${CMAKE_CURRENT_LIST_DIR}/vector.hpp
        ${CMAKE_CURRENT_LIST_DIR}/matrix.hpp
        ${CMAKE_CURRENT_LIST_DIR}/quaternion.hpp
        ${CMAKE_CURRENT_LIST_DIR}/random.hpp)

include(${CMAKE_CURRENT_LIST_DIR}/detail/CMakeLists.txt)
