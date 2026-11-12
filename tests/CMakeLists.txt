# vkm_log's own tests: TEST("what works") in tests/<topic>.cpp (harness/check.h). Each runs in a
# process of its own and is a ctest of its own, named "vkm_log: <what works>" and labeled vkm_log, so
# `ctest -L vkm_log` runs the suite in the tree of a consumer that calls enable_testing() too, with
# -C <config> under a multi-config generator. It needs no other library.

file(GLOB VKM_LOG_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(vkm_log_tests ${CMAKE_CURRENT_SOURCE_DIR}/harness/main.cpp ${VKM_LOG_TEST_SOURCES})
target_link_libraries(vkm_log_tests PRIVATE ${PROJECT_NAME})
# Beside the library, where Windows looks for the DLL a program needs.
set_target_properties(vkm_log_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:${PROJECT_NAME}>)
target_include_directories(vkm_log_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(vkm_log_tests PRIVATE VKM_TEST_SUITE="vkm_log")
target_compile_options(vkm_log_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wshadow -Wformat=2>
)

# Every test the binary lists becomes a ctest when ctest runs: a test added is a test run, with no
# list to keep.
include(${CMAKE_CURRENT_SOURCE_DIR}/harness/discover.cmake)
vkm_discover_tests(vkm_log_tests vkm_log)
