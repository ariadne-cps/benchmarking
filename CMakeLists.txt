cmake_minimum_required(VERSION 3.15)
project(Benchmarking VERSION 3.0 LANGUAGES CXX)

option(WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
if(NOT TARGET ariadne-build)
    find_package(Git REQUIRED)
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env "LC_ALL=C"
                "${GIT_EXECUTABLE}" -C "${CMAKE_CURRENT_SOURCE_DIR}"
                submodule update --init -- ss/bui
        RESULT_VARIABLE _result)
    if(_result)
        message(FATAL_ERROR "Cannot initialize Build submodule")
    endif()
    include("${CMAKE_CURRENT_SOURCE_DIR}/ss/bui/cmake/ProjectOptions.cmake")
endif()

# Validate functional and active auxiliary pins only when Benchmarking is
# the compilation root. In an embedded build, the parent's validated commits
# are authoritative; no nested commit checks are performed.
require_same_dependency_commit(Build CHAINS
    "build" "logging|build" "testing|build")
setup_project()
add_subdirectory(ss)

if(NOT TARGET benchmarking)
    add_library(benchmarking INTERFACE)
    target_include_directories(benchmarking INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(benchmarking INTERFACE logging)
    # This target serves benchmark executables, never production modules.
    set_property(TARGET benchmarking PROPERTY ARIADNE_MODULE_ROLE AUXILIARY)
endif()

# The target and the root-scope registration API are owned by Benchmarking.
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/BenchmarkingSupport.cmake")
if(NOT TARGET benchmarks)
    add_custom_target(benchmarks)
endif()
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    # A standalone test-only archive bundles Logging and its transitive object files.
    # It is NOT exported, installed, or linked into consumers' production code.
    get_target_property(_logging_objects logging ARIADNE_PROJECT_OBJECT_TARGETS)
    add_library(ariadne-benchmarking STATIC src/benchmarking.cpp)
    foreach(_object_target IN LISTS _logging_objects)
        target_sources(ariadne-benchmarking PRIVATE
            $<TARGET_OBJECTS:${_object_target}>)
    endforeach()
    target_link_libraries(ariadne-benchmarking PUBLIC benchmarking)

    setup_standalone_project(
        TARGET ariadne-benchmarking
        TEST_DIRECTORY tests
        TEST_TARGET tests
        SOURCES "${PROJECT_SOURCE_DIR}/include"
    )
    add_subdirectory(tutorial EXCLUDE_FROM_ALL)
else()
    # Consumers link this target only from their dedicated benchmark executables.
    add_library(ariadne-benchmarking INTERFACE)
    target_link_libraries(ariadne-benchmarking INTERFACE benchmarking)
endif()

