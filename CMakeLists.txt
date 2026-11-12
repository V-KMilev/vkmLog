cmake_minimum_required(VERSION 3.21.2)

set(PROJECT_NAME vkm_log)
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 6)

project(
  ${PROJECT_NAME}
  VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
)

message("[VKM_LOG] Main object:")
message("\t- ${PROJECT_NAME}")

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(SOURCES
    ${SRC_DIR}/vkm_log/logger.cpp
    ${SRC_DIR}/vkm_log/assert.cpp
)

add_library(${PROJECT_NAME} SHARED
    ${SOURCES}
)

# The root above vkm_log/, so an include names the library: "vkm_log/logger.h". Split so
# it resolves against this tree during development and against <prefix>/include when
# vkm_log ships inside an SDK that exports it.
target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${SRC_DIR}>
    $<INSTALL_INTERFACE:include>
)

# Public: its headers are C++17 (string_view, optional, filesystem), so whatever includes them is too.
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

target_link_libraries(${PROJECT_NAME} PUBLIC
)

# The suite (tests/), built when a consumer, or this project built alone, asks for it.
option(VKM_LOG_BUILD_TESTS "Build the vkm_log test suite" OFF)
if(VKM_LOG_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
