
project(asio)

add_library(${PROJECT_NAME})

include(${CMAKE_CURRENT_LIST_DIR}/include/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/src/CMakeLists.txt)
