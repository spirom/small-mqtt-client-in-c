project(libtest C)

add_definitions(-std=c99)

set(SOURCE_FILES
        runner.c
        runner.h
        test_macros.h)

include_directories(../inc)

add_library(test STATIC ${SOURCE_FILES})