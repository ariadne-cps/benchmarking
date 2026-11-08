# Only runtime support: Logging (and its Utility dependency).
# Testing is independently initialized for the standalone tests.
if(NOT TARGET logging)
    ariadne_add_submodule(logging)
endif()
