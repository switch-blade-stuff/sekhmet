cmake_minimum_required(VERSION 3.21)
project(sekhmet)

include(${CMAKE_CURRENT_LIST_DIR}/CMakeLists.macros.txt)
include(${CMAKE_CURRENT_LIST_DIR}/CMakeLists.options.txt)

sek_set_cxx_version_20()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib CACHE STRING "")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin CACHE STRING "")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin CACHE STRING "")
include_directories(${CMAKE_CURRENT_LIST_DIR})

if (PROJECT_IS_TOP_LEVEL OR SEK_EXPORT_BUILD)
    add_library(${PROJECT_NAME} SHARED)
    sek_init_target_options(${PROJECT_NAME})

    # Set project-related variables & properties
    sek_set_maybe_parent(SEK_PROJECT ${PROJECT_NAME})
    sek_set_maybe_parent(SEK_LIBRARY_PATH $<TARGET_FILE:${PROJECT_NAME}>)
    sek_set_maybe_parent(SEK_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/sekhmet)
    # set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "lib")
    set_target_properties(${PROJECT_NAME} PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)

    # Set compilation variables & definitions
    target_compile_definitions(${PROJECT_NAME} PRIVATE SEK_EXPORT_LIBRARY)

    # Find & link thread library
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    if (Threads_FOUND)
        if (CMAKE_USE_PTHREADS_INIT)
            add_compile_definitions(SEK_USE_PTHREAD)
        endif ()

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
