cmake_minimum_required(VERSION 3.21.2)

project(vkmLog VERSION 0.0.1)

message("[VKMLOG] Main object:")
message("\t- ${PROJECT_NAME}")

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(SOURCES
    ${SRC_DIR}/logger.cpp
    ${SRC_DIR}/l_assert.cpp
)

add_library(${PROJECT_NAME} STATIC
    ${SOURCES}
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${SRC_DIR}
)

target_link_libraries(${PROJECT_NAME} PUBLIC
)
