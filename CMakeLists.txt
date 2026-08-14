cmake_minimum_required(VERSION 3.21.2)

set(PROJECT_NAME vkmLog)
set(PROJECT_VERSION_MAJOR 1)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 0)

project(
  ${PROJECT_NAME}
  VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
)

message("[VKMLOG] Main object:")
message("\t- ${PROJECT_NAME}")

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(SOURCES
    ${SRC_DIR}/logger.cpp
    ${SRC_DIR}/l_assert.cpp
)

add_library(${PROJECT_NAME} SHARED
    ${SOURCES}
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${SRC_DIR}
)

target_link_libraries(${PROJECT_NAME} PUBLIC
)
