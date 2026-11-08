include_guard(GLOBAL)

# Public root-scope benchmark registration. The Build repository invokes this
# only after the top-level project has configured its own library and selected
# the optional Benchmarking submodule. No benchmark sources are owned here.
macro(ariadne_register_project_benchmarks)
    if(NOT TARGET benchmarks)
        message(FATAL_ERROR
            "The Benchmarking repository did not create target 'benchmarks'.")
    endif()
    if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
        get_property(_benchmarks_registered GLOBAL PROPERTY
                     ARIADNE_BENCHMARKS_REGISTERED)
        if(NOT _benchmarks_registered
           AND EXISTS "${PROJECT_SOURCE_DIR}/benchmarks/CMakeLists.txt")
            add_subdirectory("${PROJECT_SOURCE_DIR}/benchmarks" EXCLUDE_FROM_ALL)
            set_property(GLOBAL PROPERTY ARIADNE_BENCHMARKS_REGISTERED TRUE)
        endif()
    endif()
endmacro()
