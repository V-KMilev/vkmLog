cmake_minimum_required(VERSION 3.25.0)

set(PROJECT_NAME vkm_log)
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 6)

project(
  ${PROJECT_NAME}
  VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

message("[VKM_LOG] Building vkm_log v${PROJECT_VERSION}")

# Enumerated as one sweep rather than as a list: a forgotten entry in a list is a missing symbol
# somewhere else.
file(GLOB_RECURSE VKM_LOG_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

# Shared: the logger is one per process, and a consumer may link it into more than one shared
# library of its own; linked statically into each, every one would hold a logger of its own.
add_library(${PROJECT_NAME} SHARED ${VKM_LOG_SOURCES})

# The public surface carries no __declspec(dllexport), so on Windows CMake generates the export
# table from the object files; without it an MSVC build links a DLL that exports nothing.
set_target_properties(${PROJECT_NAME} PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

# The root above vkm_log/, so an include names the library: "vkm_log/logger.h". Split so it
# resolves against this tree during development and against <prefix>/include in an installed SDK.
target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)

# Public: its headers are C++17 (string_view, optional, filesystem), so whatever includes them is too.
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

# Its warning set. Clang reads the doc blocks: a @param naming no parameter is an error.
set(_vkm_log_warnings
    -Wall
    -Wextra
    -Wshadow
    -Wformat=2
    -Wnon-virtual-dtor
    -Woverloaded-virtual
)
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:${_vkm_log_warnings}>
    $<$<CXX_COMPILER_ID:Clang>:-Werror=documentation>
)

# The suite (tests/), built when a consumer, or this project built alone, asks for it.
option(VKM_LOG_BUILD_TESTS "Build the vkm_log test suite" OFF)
if(VKM_LOG_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
