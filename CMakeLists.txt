cmake_minimum_required(VERSION 3.20)
project(sekhmet)

if (NOT ${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
    set(SEK_TOP_LEVEL OFF)
else ()
    set(SEK_TOP_LEVEL ON)
endif ()

macro(sek_set_maybe_parent)
    if (SEK_TOP_LEVEL)
        set(${ARGV0} ${ARGV1})
    else ()
        set(${ARGV0} ${ARGV1} PARENT_SCOPE)
    endif ()
endmacro()

macro(sek_init_target_options)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
            target_compile_options(${ARGV0} PRIVATE /W4 /wd4251)
        else()
            target_compile_options(${ARGV0} PRIVATE /W3)
        endif()
    endif ()

    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        target_compile_options(${ARGV0} PRIVATE "-Wall" "-Wextra" "-Wno-unknown-pragmas" "-Wconversion" "-Werror")
        target_compile_options(${ARGV0} PRIVATE "-march=native")

        if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
            # target_compile_options(${ARGV0} PRIVATE "-fconcepts-diagnostics-depth=32")
            target_compile_options(${ARGV0} PRIVATE "-fconcepts-diagnostics-depth=32")
        else ()
            target_compile_options(${ARGV0} PRIVATE "-ffile-prefix-map=\"${CMAKE_CURRENT_LIST_DIR}/\"=\"\"")
        endif ()
    endif ()
endmacro()

macro(sek_init_tool_subproject)
    sek_init_target_options(${ARGV0})
    set_target_properties(${ARGV0} PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/tools"
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/tools"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tools")
endmacro()

# Handle pre C++20 ABI-freeze MSVC versions
if (MSVC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS_EQUAL 19.30)
    set(CMAKE_CXX_STANDARD 23) # /std:c++latest required for MSVC pre abi-freeze
else ()
    set(CMAKE_CXX_STANDARD 20)
endif ()

include(${CMAKE_CURRENT_LIST_DIR}/CMakeLists.options.txt)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_SYSTEM_PROCESSOR}/lib CACHE STRING "")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_SYSTEM_PROCESSOR}/bin CACHE STRING "")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_SYSTEM_PROCESSOR}/bin CACHE STRING "")
include_directories(${CMAKE_CURRENT_LIST_DIR})

if (SEK_TOP_LEVEL OR SEK_EXPORT_BUILD)
    add_library(${PROJECT_NAME} SHARED)
    sek_init_target_options(${PROJECT_NAME})

    # Set project-related variables & properties
    sek_set_maybe_parent(SEK_PROJECT ${PROJECT_NAME})
    sek_set_maybe_parent(SEK_LIBRARY_PATH $<TARGET_FILE:${PROJECT_NAME}>)
    sek_set_maybe_parent(SEK_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/sekhmet)
    sek_set_maybe_parent(SEK_ENGINE_VERSION 0.0.1)
    set_target_properties(${PROJECT_NAME} PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)

    # Set compilation variables & definitions
    target_compile_definitions(${PROJECT_NAME} PRIVATE SEK_EXPORT_LIBRARY)
    add_compile_definitions(SEK_ENGINE_VERSION="${SEK_ENGINE_VERSION}")

    # Find & link thread library
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    if (Threads_FOUND)
        set(SEK_THREADS_LIBRARY ${CMAKE_THREAD_LIBS_INIT})
        target_link_libraries(${PROJECT_NAME} PUBLIC ${SEK_THREADS_LIBRARY})
    endif ()

    # Include project source subdirectory
    include(${SEK_SOURCE_DIR}/CMakeLists.txt)

    # Set project include file for all subsequent projects
    set(CMAKE_PROJECT_INCLUDE ${CMAKE_CURRENT_LIST_DIR}/CMakeLists.include.txt)

    # Include editor project
    if (SEK_BUILD_EDITOR AND NOT SEK_EXPORT_BUILD AND EXISTS ${SEK_SOURCE_DIR}/editor/CMakeLists.txt)
        add_compile_definitions(SEK_EDITOR)
        add_subdirectory(${SEK_SOURCE_DIR}/editor editor)
    endif ()

    # Include modules subdirectory
    if (SEK_BUILD_MODULES)
        include(${CMAKE_CURRENT_LIST_DIR}/modules/CMakeLists.txt)
    endif ()

    # Include test project
    if (SEK_ENABLE_TESTS AND EXISTS ${CMAKE_CURRENT_LIST_DIR}/test/CMakeLists.txt)
        add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/test test)
    endif ()
endif ()
message("Install prefix: ${CMAKE_INSTALL_PREFIX}")