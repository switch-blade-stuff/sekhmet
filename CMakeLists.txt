cmake_minimum_required(VERSION 3.20)
project(sekhmet-core)

# Set project-related variables & properties
set(SEK_ENGINE_PROJECT ${PROJECT_NAME})
set(SEK_ENGINE_LIBRARY_PATH $<TARGET_FILE:${PROJECT_NAME}>)
set(SEK_ENGINE_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/sekhmet)
set(SEK_ENGINE_VERSION 0.0.1)

# Include engine library subdirectory
include(${SEK_ENGINE_SOURCE_DIR}/CMakeLists.txt)