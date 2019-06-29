
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

void
init_result(test_result_t *result)
{
    result->success = true;
    result->line = 0;
}

char *server_host = "127.0.0.1";

const char *get_server_host() 
{
    return server_host;
}

int
main (int argc, char** argv) {

    if (argc > 1) {
	server_host = argv[1];
    }
    fprintf(stdout, "Server-based tests will run against host [%s]\n", get_server_host());

    size_t test_count = sizeof(all_tests) / sizeof(test_definition_t);
    uint16_t skip_count = 0;
    uint16_t fail_count = 0;

    for (uint8_t i = 0; i < test_count && all_tests[i].function != NULL; i++) {
        test_definition_t *test = all_tests + i;
        fprintf(stdout, "[%3u] %-40s ", i + 1, test->name);
        test_result_t result = (test->function)();
        fprintf(stdout, "%s \n", result.success ? "PASS" : "FAIL");
        if (!result.success) {
            fprintf(stdout, "........ Line: %d\n", result.line);
            fprintf(stdout, "........ Condition: %s\n", result.e);
            fail_count++;
        }
    }
    if (fail_count == 0) {
        fprintf(stdout, "All %lu tests passed\n", test_count);
    } else {
        fprintf(stdout, "%d of %lu tests failed\n", fail_count, test_count);
    }
    if (skip_count != 0) {
        fprintf(stderr, "%d tests skipped\n", skip_count);
    } else {
        fprintf(stdout, "No tests were skipped\n");
    }

    return 0;
}


