find_package(GTest)

if (GTest_FOUND)
    project(${PROJECT_NAME}.test)
    set(CMAKE_CXX_STANDARD 20)

    include(GoogleTest)

    add_executable(${PROJECT_NAME}
            main.cpp
            utility_tests.cpp
            debug_tests.cpp
            container_tests.cpp
            adapter_tests.cpp
            reflection_tests.cpp
            plugin_tests.cpp
            adt_tests.cpp
            math_tests.cpp)
    target_link_libraries(${PROJECT_NAME} PUBLIC
            ${SEKHMET_PROJECT}
            dl GTest::GTest)
else ()
    message("\nWarning: GTest not found, unit tests would not be built\n")
endif ()
