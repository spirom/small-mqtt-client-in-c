
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "messages.h"
#include "messages_internal.h"
#include "smqttc_internal.h"
#include "smqttc.h"
#include "test_macros.h"

void fill_buffer(uint8_t *buffer)
{
    buffer[0] = 0xFF;
    buffer[1] = 0xFF;
    buffer[2] = 0xFF;
    buffer[3] = 0xFF;
}

test_result_t
test_length_encoding()
{
    test_result_t result;
    init_result(&result);

    bool ret;
    uint8_t buffer[4];
    uint8_t used;

    fill_buffer(buffer);
    used = 0;
    ret = encode_size(0, buffer, &used);
    ASSERT_TRUE(ret, result);
    ASSERT_TRUE(buffer[0] == 0, result);
    ASSERT_TRUE(used == 1, result);

    fill_buffer(buffer);
    used = 0;
    ret = encode_size(127, buffer, &used);
    ASSERT_TRUE(ret, result);
    ASSERT_TRUE(buffer[0] == 0x7F, result);
    ASSERT_TRUE(used == 1, result);

    fill_buffer(buffer);
    used = 0;
    ret = encode_size(128, buffer, &used);
    ASSERT_TRUE(ret, result);
    ASSERT_TRUE(buffer[0] == 0x80, result);
    ASSERT_TRUE(buffer[1] == 0x01, result);
    ASSERT_TRUE(used == 2, result);

    fill_buffer(buffer);
    used = 0;
    ret = encode_size(16383, buffer, &used);
    ASSERT_TRUE(ret, result);
    ASSERT_TRUE(buffer[0] == 0xFF, result);
    ASSERT_TRUE(buffer[1] == 0x7F, result);
    ASSERT_TRUE(used == 2, result);

    fill_buffer(buffer);
    used = 0;
    ret = encode_size(16384, buffer, &used);
    ASSERT_TRUE(ret, result);
    ASSERT_TRUE(buffer[0] == 0x80, result);
    ASSERT_TRUE(buffer[1] == 0x80, result);
    ASSERT_TRUE(buffer[2] == 0x01, result);
    ASSERT_TRUE(used == 3, result);

    fill_buffer(buffer);
    used = 0;
    ret = encode_size(2097151, buffer, &used);
    ASSERT_TRUE(ret, result);
    ASSERT_TRUE(buffer[0] == 0xFF, result);
    ASSERT_TRUE(buffer[1] == 0xFF, result);
    ASSERT_TRUE(buffer[2] == 0x7F, result);
    ASSERT_TRUE(used == 3, result);

    fill_buffer(buffer);
    used = 0;
    ret = encode_size(2097152, buffer, &used);
    ASSERT_TRUE(ret, result);
    ASSERT_TRUE(buffer[0] == 0x80, result);
    ASSERT_TRUE(buffer[1] == 0x80, result);
    ASSERT_TRUE(buffer[2] == 0x80, result);
    ASSERT_TRUE(buffer[3] == 0x01, result);
    ASSERT_TRUE(used == 4, result);

    fill_buffer(buffer);
    used = 0;
    ret = encode_size(268435455, buffer, &used);
    ASSERT_TRUE(ret, result);
    ASSERT_TRUE(buffer[0] == 0xFF, result);
    ASSERT_TRUE(buffer[1] == 0xFF, result);
    ASSERT_TRUE(buffer[2] == 0xFF, result);
    ASSERT_TRUE(buffer[3] == 0x7F, result);
    ASSERT_TRUE(used == 4, result);

    return result;
}

test_result_t
test_length_decoding()
{
    test_result_t result;
    init_result(&result);

    bool ret;
    uint8_t used;
    size_t size;

    uint8_t buffer_1[] = { 0x00 };
    used = 0;
    size = 1;
    ret = decode_size(buffer_1, buffer_1, &size, &used);
    ASSERT_TRUE(ret, result);
    ASSERT_TRUE(size == 0, result);
    ASSERT_TRUE(used == 1, result);

    uint8_t buffer_2[] = { 0x7F };
    used = 0;
    size = 0;
    ret = decode_size(buffer_2, buffer_2, &size, &used);
    ASSERT_TRUE(ret, result);
    ASSERT_TRUE(size == 127, result);
    ASSERT_TRUE(used == 1, result);

    uint8_t buffer_3[] = { 0x80, 0x01 };
    used = 0;
    size = 0;
    ret = decode_size(buffer_3, buffer_3 + 1, &size, &used);
    ASSERT_TRUE(ret, result);
    ASSERT_TRUE(size == 128, result);
    ASSERT_TRUE(used == 2, result);

    uint8_t buffer_4[] = { 0xFF, 0x7F };
    used = 0;
    size = 0;
    ret = decode_size(buffer_4, buffer_4 + 1, &size, &used);
    ASSERT_TRUE(ret, result);
    ASSERT_TRUE(size == 16383, result);
    ASSERT_TRUE(used == 2, result);

    uint8_t buffer_5[] = { 0x80, 0x80, 0x01 };
    used = 0;
    size = 0;
    ret = decode_size(buffer_5, buffer_5 + 2, &size, &used);
    ASSERT_TRUE(ret, result);
    ASSERT_TRUE(size == 16384, result);
    ASSERT_TRUE(used == 3, result);

    uint8_t buffer_6[] = { 0xFF, 0xFF, 0x7F };
    used = 0;
    size = 0;
    ret = decode_size(buffer_6, buffer_6 + 2, &size, &used);
    ASSERT_TRUE(ret, result);
    ASSERT_TRUE(size == 2097151, result);
    ASSERT_TRUE(used == 3, result);

    uint8_t buffer_7[] = { 0x80, 0x80, 0x80, 0x01 };
    used = 0;
    size = 0;
    ret = decode_size(buffer_7, buffer_7 + 3, &size, &used);
    ASSERT_TRUE(ret, result);
    ASSERT_TRUE(size == 2097152, result);
    ASSERT_TRUE(used == 4, result);

    uint8_t buffer_8[] = { 0xFF, 0xFF, 0xFF, 0x7F };
    used = 0;
    size = 0;
    ret = decode_size(buffer_8, buffer_8 + 3, &size, &used);
    ASSERT_TRUE(ret, result);
    ASSERT_TRUE(size == 268435455, result);
    ASSERT_TRUE(used == 4, result);


    return result;
}

