#ifndef SMQTTC_RUNNER_H
#define SMQTTC_RUNNER_H

#include "test_macros.h"

extern void
init_result(test_result_t *result);

extern int
run_tests(test_definition_t tests[], uint16_t test_count);

#endif //SMQTTC_RUNNER_H
