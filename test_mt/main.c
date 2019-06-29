
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "runner.h"

#include "../libtest/test_macros.h"

#define TEST_DEF(t) test_result_t t();

#include "test_definitions.txt"

#undef TEST_DEF
#define TEST_DEF(t) { #t, &t },

static test_definition_t all_tests[] = {
#include "test_definitions.txt"
        { NULL, NULL } // easy way to deal with last comma
};


int
main (int argc, char** argv) {
    return run_tests(all_tests,
            (sizeof(all_tests) - 1) / sizeof(test_definition_t));
}


