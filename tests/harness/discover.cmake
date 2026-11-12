# A suite's tests as ctests, in two steps. A suite's CMakeLists.txt includes this file and calls
# vkm_discover_tests(<target> <suite> [<label>...]), which writes the list ctest reads; when ctest runs,
# that list includes this file again, with VKM_TEST_EXE the binary, VKM_TEST_TARGET its target,
# VKM_TEST_SUITE the suite's name, VKM_TEST_LABELS the labels beside it and VKM_TEST_CMAKE cmake's path,
# to ask the binary for its tests and add each as a ctest of its own, labeled with the suite and those
# labels. A test that stands for a binary that cannot answer names the target, as one suite may have
# two binaries.

if(NOT DEFINED VKM_TEST_EXE)
    # A multi-config generator builds the binary once per build type, so each gets a list of its own,
    # named in lower case as ctest -C matches a build type in any case, and the file ctest includes
    # reads the one -C names, or fails one test when -C names none of them, so the run goes on. A
    # single-config generator's one list is that file itself.
    function(vkm_discover_tests target suite)
        set(_labels ${suite} ${ARGN})
        set(_include ${CMAKE_CURRENT_BINARY_DIR}/${target}.cmake)
        set(_list ${_include})
        get_property(_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
        if(_multi_config)
            set(_list ${CMAKE_CURRENT_BINARY_DIR}/${target}-$<LOWER_CASE:$<CONFIG>>.cmake)
            string(REPLACE ";" ", " _types "${CMAKE_CONFIGURATION_TYPES}")
            file(WRITE ${_include}
                "string(TOLOWER \"\${CTEST_CONFIGURATION_TYPE}\" _vkm_config)\n"
                "set(_vkm_list \"${CMAKE_CURRENT_BINARY_DIR}/${target}-\${_vkm_config}.cmake\")\n"
                "if(_vkm_config STREQUAL \"\" OR NOT EXISTS \"\${_vkm_list}\")\n"
                "    set(_vkm_named \"${suite}: ctest -C names a build type ${target} has\")\n"
                "    set(_vkm_why \"run ctest -C, one of ${_types}\")\n"
                "    add_test(\"\${_vkm_named}\" [==[${CMAKE_COMMAND}]==] -E echo \"\${_vkm_why}\")\n"
                "    set_tests_properties(\"\${_vkm_named}\" PROPERTIES\n"
                "        WILL_FAIL ON LABELS [==[${_labels}]==])\n"
                "    return()\n"
                "endif()\n"
                "include(\"\${_vkm_list}\")\n"
            )
        endif()
        string(CONCAT _content
            "set(VKM_TEST_EXE [==[$<TARGET_FILE:${target}>]==])\n"
            "set(VKM_TEST_TARGET ${target})\n"
            "set(VKM_TEST_SUITE ${suite})\n"
            "set(VKM_TEST_LABELS [==[${ARGN}]==])\n"
            "set(VKM_TEST_CMAKE [==[${CMAKE_COMMAND}]==])\n"
            "include([==[${CMAKE_CURRENT_FUNCTION_LIST_FILE}]==])\n"
        )
        file(GENERATE OUTPUT ${_list} CONTENT "${_content}")
        set_property(DIRECTORY APPEND PROPERTY TEST_INCLUDE_FILES ${_include})
    endfunction()
    return()
endif()

set(_vkm_labels ${VKM_TEST_SUITE} ${VKM_TEST_LABELS})
set(_vkm_tests "")

if(NOT EXISTS "${VKM_TEST_EXE}")
    # Listed as a failure, not dropped: a suite that did not build is not a suite that passed.
    set(_vkm_built "${VKM_TEST_SUITE}: ${VKM_TEST_TARGET} is built")
    add_test("${_vkm_built}" "${VKM_TEST_CMAKE}" -E echo "${VKM_TEST_EXE} is not built")
    set_tests_properties("${_vkm_built}" PROPERTIES WILL_FAIL ON LABELS "${_vkm_labels}")
    return()
endif()

execute_process(
    COMMAND "${VKM_TEST_EXE}" --list
    OUTPUT_VARIABLE _vkm_tests
    RESULT_VARIABLE _vkm_status
    TIMEOUT 60
)
if(NOT _vkm_status EQUAL 0)
    set(_vkm_lists "${VKM_TEST_SUITE}: ${VKM_TEST_TARGET} lists its tests")
    add_test("${_vkm_lists}" "${VKM_TEST_EXE}" --list)
    set_tests_properties("${_vkm_lists}" PROPERTIES LABELS "${_vkm_labels}")
    return()
endif()

# Each test runs in the binary's folder, above its scratch, so a relative path a test uses resolves
# the same whatever folder the consumer's ctest adds it from.
get_filename_component(_vkm_folder "${VKM_TEST_EXE}" DIRECTORY)
string(REPLACE "\n" ";" _vkm_tests "${_vkm_tests}")
foreach(_vkm_test IN LISTS _vkm_tests)
    if(_vkm_test STREQUAL "")
        continue()
    endif()
    add_test("${_vkm_test}" "${VKM_TEST_EXE}" "${_vkm_test}")
    set_tests_properties("${_vkm_test}" PROPERTIES
        LABELS            "${_vkm_labels}"
        SKIP_RETURN_CODE  77
        TIMEOUT           300
        WORKING_DIRECTORY "${_vkm_folder}"
    )
endforeach()
