# Native unit tests. Each test is one executable built from one tests/test_*.c
# file plus the product sources it exercises. Product sources listed here must
# be portable (no <windows.h>, no SDL); put platform code behind a seam.
#
#   cyber_test(<name> <test source> [product sources...])
#
# Run:  ctest --test-dir <build dir> --output-on-failure

set(CYBER_SRC ${PROJECT_SOURCE_DIR}/src)

function(cyber_test name)
    add_executable(${name} ${ARGN})
    target_include_directories(${name} PRIVATE ${CYBER_SRC} ${CMAKE_CURRENT_SOURCE_DIR})
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${name} PRIVATE -Wall -Wextra)
    endif()
    if(MINGW)
        target_compile_definitions(${name} PRIVATE _WIN32_WINNT=0x0501 WINVER=0x0501)
        target_link_options(${name} PRIVATE -static-libgcc)
    endif()
    add_test(NAME ${name} COMMAND ${name})
endfunction()

cyber_test(test_util test_util.c ${CYBER_SRC}/util.c)
cyber_test(test_library test_library.c fake_fs.c ${CYBER_SRC}/library.c ${CYBER_SRC}/util.c)
cyber_test(test_config test_config.c ${CYBER_SRC}/config.c ${CYBER_SRC}/util.c)
cyber_test(test_pad test_pad.c ${CYBER_SRC}/pad.c)
cyber_test(test_redraw test_redraw.c ${CYBER_SRC}/redraw.c)
cyber_test(test_attract test_attract.c ${CYBER_SRC}/attract.c)
