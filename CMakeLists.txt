cmake_minimum_required(VERSION 3.21.2)

set(PROJECT_NAME vkm_log)
set(PROJECT_VERSION_MAJOR 1)
set(PROJECT_VERSION_MINOR 1)
set(PROJECT_VERSION_PATCH 0)

project(
  ${PROJECT_NAME}
  VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
)

message("[VKM_LOG] Main object:")
message("\t- ${PROJECT_NAME}")

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(SOURCES
    ${SRC_DIR}/logger.cpp
    ${SRC_DIR}/l_assert.cpp
)

add_library(${PROJECT_NAME} SHARED
    ${SOURCES}
)

# Split so the same include resolves against this tree during development and
# against <prefix>/include when vkm_log ships inside an SDK that exports it.
target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${SRC_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT_NAME} PUBLIC
)
