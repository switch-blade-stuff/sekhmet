
target_sources(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/toml_archive.cpp)

target_sources(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/ubj/ubj_archive.cpp
        ${CMAKE_CURRENT_LIST_DIR}/ubj/ubj_spec12_types.c)