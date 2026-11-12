# A LOG_* or VKM_ASSERT call whose arguments do not fit its format is caught by the compiler: the
# fixture compiles clean as written, and fails under -Werror=format with either call given arguments
# its format does not take (cmake -P, -DCXX=<compiler> -DSOURCE_DIR=<src> -DFIXTURE=<fixture>).

foreach(_misfit NONE LOG ASSERT)
    set(_defines "")
    if(NOT _misfit STREQUAL NONE)
        set(_defines -DVKM_TEST_MISFIT_${_misfit})
    endif()
    execute_process(
        COMMAND "${CXX}" -std=c++17 -fsyntax-only -Werror=format -I "${SOURCE_DIR}" ${_defines} "${FIXTURE}"
        RESULT_VARIABLE _status
        ERROR_VARIABLE _errors
    )
    if(_misfit STREQUAL NONE AND NOT _status EQUAL 0)
        message(FATAL_ERROR "The fixture as written does not compile:\n${_errors}")
    endif()
    # The format warning by its flag: GCC's -Werror=format=, Clang's -Wformat. Any other error fails too.
    if(NOT _misfit STREQUAL NONE AND (_status EQUAL 0 OR NOT _errors MATCHES "-Werror=format|-Wformat"))
        message(FATAL_ERROR "A ${_misfit} call that does not fit its format did not fail on it:\n${_errors}")
    endif()
endforeach()
