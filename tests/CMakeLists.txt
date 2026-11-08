include(CTest)
add_executable(test_benchmarking test_benchmarking.cpp)
target_link_libraries(test_benchmarking PRIVATE ariadne-benchmarking ariadne-testing)
add_test(NAME test_benchmarking COMMAND $<TARGET_FILE:test_benchmarking>)
if(WIN32)
    set_tests_properties(test_benchmarking PROPERTIES
        ENVIRONMENT_MODIFICATION
        "PATH=path_list_prepend:$<TARGET_FILE_DIR:ariadne-benchmarking>")
endif()
add_custom_target(tests)
add_dependencies(tests test_benchmarking)
