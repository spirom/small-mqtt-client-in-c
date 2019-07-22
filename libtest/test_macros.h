
#ifndef SMQTTC_TEST_MACROS_H
#define SMQTTC_TEST_MACROS_H

#define MAX_TEXT 1000


#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#define BUFFER_SIZE 1024

#define ASSERT_TRUE(expr, result) \
    do { \
        if (result.success && !(expr)) { \
            result.success = false; \
            result.line = __LINE__; \
            strncpy(result.e, #expr, MAX_TEXT); \
        } \
    } while (0)

#define ASSERT_FALSE(expr, result) \
    do { \
        if (result.success && (expr)) { \
            result.success = false; \
            result.line = __LINE__; \
            strncpy(result.e, #expr, MAX_TEXT); \
        } \
    } while (0)

typedef struct test_result_t {
    bool            success;
    int             line;
    char            e[MAX_TEXT];
} test_result_t;

typedef struct test_definition_t {
    const char *name;
    test_result_t (*function)();
} test_definition_t;

void
init_result(test_result_t *result);

const char *
get_server_host();


#endif //SMQTTC_TEST_MACROS_H
