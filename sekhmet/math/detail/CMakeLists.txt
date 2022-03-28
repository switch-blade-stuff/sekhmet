
if (MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /EHa")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHa")
endif ()

include(${CMAKE_CURRENT_LIST_DIR}/CMake.options.txt)

target_sources(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/sysrandom.cpp)