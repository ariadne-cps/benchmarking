include(CTest)
add_custom_target(tests)
foreach(component statistics command_line reporting comparison parameters runner)
    set(test_target test_${component})
    add_executable(${test_target} ${test_target}.cpp)
    target_link_libraries(${test_target} PRIVATE ariadne-benchmarking ariadne-testing)
    add_test(NAME ${test_target} COMMAND $<TARGET_FILE:${test_target}> --concise)
    if(WIN32)
        set_tests_properties(${test_target} PROPERTIES
            ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:ariadne-benchmarking>")
    endif()
    add_dependencies(tests ${test_target})
endforeach()
