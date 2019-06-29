
#include "test_macros.h"


void
init_result(test_result_t *result)
{
    result->success = true;
    result->line = 0;
}

int
run_tests(test_definition_t tests[], uint16_t test_count) {
    uint16_t skip_count = 0;
    uint16_t fail_count = 0;

    for (uint8_t i = 0; i < test_count && tests[i].function != NULL; i++) {
        test_definition_t *test = tests + i;
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
        fprintf(stdout, "All %u tests passed\n", test_count);
    } else {
        fprintf(stdout, "%d of %u tests failed\n", fail_count, test_count);
    }
    if (skip_count != 0) {
        fprintf(stderr, "%d tests skipped\n", skip_count);
    } else {
        fprintf(stdout, "No tests were skipped\n");
    }

    return 0;
}


