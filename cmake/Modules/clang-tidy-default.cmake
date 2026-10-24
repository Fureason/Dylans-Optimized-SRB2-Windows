find_program(CLANG_TIDY clang-tidy)






function(target_set_default_clang_tidy target lang checks)
    if("${CLANG_TIDY}" STREQUAL "CLANG_TIDY-NOTFOUND")
        return()
    endif()

    get_target_property(c_clang_tidy_prop SRB2SDL2 C_CLANG_TIDY)
    if(NOT ("${c_clang_tidy_prop}" STREQUAL "c_clang_tidy_prop-NOTFOUND"))
        return()
    endif()

    set_target_properties("${target}" PROPERTIES
	    ${lang}_CLANG_TIDY "${CLANG_TIDY};-checks=${checks}"
    )
endfunction()
