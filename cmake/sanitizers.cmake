option(UBSAN OFF "Enable UB sanitizer")
if (UBSAN)
    add_compile_options(-fsanitize=undefined)
    add_link_options(-fsanitize=undefined)
endif()

option(ASAN OFF "Enable address sanitizer")
if (ASAN)
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
endif()

option(TSAN OFF "Enable thread sanitizer")
if (TSAN)
    add_compile_options(-fsanitize=thread)
    add_link_options(-fsanitize=thread)
endif()
