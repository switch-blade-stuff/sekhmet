
if (ASIO_COMPILE_SRC)
    add_compile_definitions(ASIO_SEPARATE_COMPILATION)

    target_sources(${PROJECT_NAME} PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/asio.cpp
            ${CMAKE_CURRENT_LIST_DIR}/asio_ssl.cpp)
endif ()