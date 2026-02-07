if(NOT TARGET polyclipping::polyclipping)
    add_library(polyclipping::polyclipping SHARED IMPORTED)
    set_target_properties(polyclipping::polyclipping PROPERTIES
        IMPORTED_LOCATION "/usr/lib/x86_64-linux-gnu/libpolyclipping.so"
        INTERFACE_INCLUDE_DIRECTORIES "/usr/include"
    )
endif()
