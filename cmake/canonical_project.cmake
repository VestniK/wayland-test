macro(cpp_unit)
    set(opts "")
    set(oneval_args NAME STD TYPE)
    set(multyval_args LIBS TEST_LIBS TEST_ARGS)
    cmake_parse_arguments(UNIT_PRJ
        "${opts}"
        "${oneval_args}"
        "${multyval_args}"
        ${ARGN}
    )
    if (NOT UNIT_PRJ_STD)
        set(UNIT_PRJ_STD cxx_std_17)
    endif()
    if (NOT UNIT_PRJ_TYPE)
        set(UNIT_PRJ_TYPE "AUTO")
    endif()
    set(LIB_PRJ_TGT ${UNIT_PRJ_NAME})

    file(GLOB GLOB_SRCS CONFIGURE_DEPENDS *.cpp)
    file(GLOB GLOB_HDRS CONFIGURE_DEPENDS *.hpp)

    # APP
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
        add_executable(${UNIT_PRJ_NAME} main.cpp)
        set(LIB_PRJ_TGT ${UNIT_PRJ_NAME}.lib)
    endif()

    # IMPL lib
    set(PRJ_IMPL_SRC ${GLOB_SRCS} ${GLOB_HDRS})
    list(FILTER PRJ_IMPL_SRC EXCLUDE REGEX "^.*/main.cpp$")
    list(FILTER PRJ_IMPL_SRC EXCLUDE REGEX "^main.cpp$")
    list(FILTER PRJ_IMPL_SRC EXCLUDE REGEX "^.*\.test.[hc]pp$")

    set(PRJ_COPMILE_SRC ${GLOB_SRCS})
    list(FILTER PRJ_COPMILE_SRC EXCLUDE REGEX "^.*/main.cpp$")
    list(FILTER PRJ_COPMILE_SRC EXCLUDE REGEX "^main.cpp$")
    list(FILTER PRJ_COPMILE_SRC EXCLUDE REGEX "^.*\.test.[hc]pp$")

    if (PRJ_COPMILE_SRC)
        if (UNIT_PRJ_TYPE STREQUAL "STATIC")
            add_library(${LIB_PRJ_TGT} STATIC)
        elseif (UNIT_PRJ_TYPE STREQUAL "SHARED")
            add_library(${LIB_PRJ_TGT} SHARED)
        elseif (UNIT_PRJ_TYPE STREQUAL "MODULE")
            add_library(${LIB_PRJ_TGT} MODULE)
        elseif (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
            add_library(${LIB_PRJ_TGT} OBJECT)
        else()
            add_library(${LIB_PRJ_TGT})
        endif()
        target_sources(${LIB_PRJ_TGT} PRIVATE ${PRJ_IMPL_SRC})
        set_target_properties(${LIB_PRJ_TGT} PROPERTIES LINKER_LANGUAGE CXX)
        target_compile_features(${LIB_PRJ_TGT} PUBLIC ${UNIT_PRJ_STD})
        target_include_directories(${LIB_PRJ_TGT}
          PUBLIC
            ${PROJECT_SOURCE_DIR}
            ${PROJECT_BINARY_DIR}
        )
        if (UNIT_PRJ_LIBS)
            target_link_libraries(${LIB_PRJ_TGT} PUBLIC ${UNIT_PRJ_LIBS})
        endif()
    else()
        add_library(${LIB_PRJ_TGT} INTERFACE)
        if (PRJ_IMPL_SRC)
            target_sources(${LIB_PRJ_TGT} PRIVATE ${PRJ_IMPL_SRC})
        endif()
        target_compile_features(${LIB_PRJ_TGT} INTERFACE ${UNIT_PRJ_STD})
        target_include_directories(${LIB_PRJ_TGT}
          INTERFACE
            ${PROJECT_SOURCE_DIR}
            ${PROJECT_BINARY_DIR}
        )
        if (UNIT_PRJ_LIBS)
            target_link_libraries(${LIB_PRJ_TGT} INTERFACE ${UNIT_PRJ_LIBS})
        endif()
    endif()

    # TESTS
    set(PRJ_TEST_SRC ${GLOB_SRCS} ${GLOB_HDRS})
    list(FILTER PRJ_TEST_SRC INCLUDE REGEX "^.*\.test.[hc]pp$")

    if (PRJ_TEST_SRC)
        add_executable(${UNIT_PRJ_NAME}.test)
        target_sources(${UNIT_PRJ_NAME}.test PRIVATE ${PRJ_TEST_SRC})
        target_link_libraries(${UNIT_PRJ_NAME}.test PRIVATE ${LIB_PRJ_TGT} ${UNIT_PRJ_TEST_LIBS})
        add_test(NAME ${UNIT_PRJ_NAME}-test COMMAND ${UNIT_PRJ_NAME}.test ${UNIT_PRJ_TEST_ARGS})
    endif()

    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
        target_link_libraries(${UNIT_PRJ_NAME} PRIVATE ${LIB_PRJ_TGT})
    endif()

endmacro()
