# Logging provides Utility transitively. Testing remains independent.
if(NOT TARGET logging)
    ariadne_add_submodule(logging)
endif()
