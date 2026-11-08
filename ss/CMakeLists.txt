# Threading supplies Logging and Utility transitively.
# Testing is separately initialized for standalone tests.
if(NOT TARGET threading)
    ariadne_add_submodule(threading)
endif()
