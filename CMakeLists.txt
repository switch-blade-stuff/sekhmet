cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)

project(sekhmet)

macro(set_maybe_parent)
    if (PROJECT_IS_TOP_LEVEL)
        set(${ARGV0} ${ARGV1})
    else ()
        set(${ARGV0} ${ARGV1} PARENT_SCOPE)
    endif ()
endmacro()

include(${CMAKE_CURRENT_LIST_DIR}/CMakeLists.options.txt)
include(${CMAKE_CURRENT_LIST_DIR}/CMakeLists.flags.txt)

include_directories(${CMAKE_CURRENT_LIST_DIR})

if (PROJECT_IS_TOP_LEVEL OR EXPORT_BUILD)
    add_library(${PROJECT_NAME} SHARED)

    # Set project-related variables & properties
    set_maybe_parent(SEKHMET_PROJECT ${PROJECT_NAME})
    set_maybe_parent(SEKHMET_LIBRARY $<TARGET_FILE:${PROJECT_NAME}>)
    set_maybe_parent(SEKHMET_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/sekhmet)
    set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "lib")

    # Set compilation variables & defenitions
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE SEK_API_EXPORT)

    # Find & link thread library
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    if (Threads_FOUND)
        if (CMAKE_USE_PTHREADS_INIT)
            add_compile_definitions(SEK_USE_PTHREAD)
        endif ()

        set(SEKHMET_THREADS_LIBRARY ${CMAKE_THREAD_LIBS_INIT})
        target_link_libraries(${PROJECT_NAME} PUBLIC ${SEKHMET_THREADS_LIBRARY})
    endif ()

    # Include project source subdirectory
    include(${SEKHMET_SOURCE_DIR}/CMakeLists.txt)

    # Include editor project
    if (BUILD_EDITOR AND NOT EXPORT_BUILD AND EXISTS ${SEKHMET_SOURCE_DIR}/editor/CMakeLists.txt)
        add_subdirectory(${SEKHMET_SOURCE_DIR}/editor editor)
    endif ()

    # Include modules subdirectory
    if (BUILD_MODULES)
        include(${CMAKE_CURRENT_LIST_DIR}/modules/CMakeLists.txt)
    endif ()

    # Include test project
    if (ENABLE_TESTS AND EXISTS ${CMAKE_CURRENT_LIST_DIR}/test/CMakeLists.txt)
        add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/test test)
    endif ()
endif ()
