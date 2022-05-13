project(${PROJECT_NAME}.test)

if (CMAKE_HOST_WIN32)
    # Windows does not have OS-level library lookup, thus need to use a local dependency
    include(FetchContent)
    FetchContent_Declare(googletest URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
else ()
    find_package(GTest)
    if (GTest_FOUND)
        if (NOT TARGET gtest)
            add_library(gtest ALIAS GTest::GTest)
        endif ()
    else ()
        set(GTEST_MISSING ON)
    endif ()
endif ()

if (NOT GTEST_MISSING)
    include(GoogleTest)
    add_executable(${PROJECT_NAME}
            main.cpp
            utility_tests.cpp
            debug_tests.cpp
            container_tests.cpp
            math_tests.cpp
            serialization_tests.cpp
            plugin_tests.cpp)
    sek_init_target_options(${PROJECT_NAME})
    target_link_libraries(${PROJECT_NAME} PUBLIC ${SEK_PROJECT} gtest)
    if (UNIX)
        target_link_libraries(${PROJECT_NAME} PUBLIC dl)
    endif ()
else ()
    message("\nWarning: GTest not found, unit tests would not be built\n")
endif ()
