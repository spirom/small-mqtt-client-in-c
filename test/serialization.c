
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <messages.h>

#include "messages.h"
#include "messages_internal.h"
#include "smqttc_internal.h"
#include "smqttc.h"
#include "test_macros.h"


test_result_t
test_connect_message_v3()
{
    test_result_t result;
    init_result(&result);

    char buffer[BUFFER_SIZE];

    uint16_t keep_alive_sec = 30u;

    size_t send_size = make_connect_message(buffer, BUFFER_SIZE, "foo", NULL, NULL,
                                            keep_alive_sec, false, false, QoS0, false, NULL, NULL);

    ASSERT_TRUE(send_size >= 0, result);

    response_t *message = deserialize_response(buffer, send_size);
    ASSERT_TRUE(message->type == CONNECT, result);
    ASSERT_TRUE(message->body.connect_data.version == 3u, result);
    ASSERT_FALSE(message->body.connect_data.has_clean, result);
    ASSERT_FALSE(message->body.connect_data.has_user, result);
    ASSERT_FALSE(message->body.connect_data.has_pw, result);
    ASSERT_FALSE(message->body.connect_data.has_will, result);
    ASSERT_TRUE(message->body.connect_data.keep_alive == keep_alive_sec, result);
    char *name = message->body.connect_data.name;
    uint16_t name_size = message->body.connect_data.name_size;
    ASSERT_TRUE(name_size == 6, result);
    ASSERT_TRUE(strncmp(name, "MQIsdp", name_size) == 0, result);

    return result;
}

test_result_t
test_connect_message_the_works_v3()
{
    test_result_t result;
    init_result(&result);

    char buffer[BUFFER_SIZE];

    uint16_t keep_alive_sec = 30u;

    size_t send_size =
            make_connect_message(buffer, BUFFER_SIZE, "foo", "fred", "mysecret",
                                 keep_alive_sec, true, true, QoS2, true, "willtopic", "willmsg");

    ASSERT_TRUE(send_size >= 0, result);

    response_t *message = deserialize_response(buffer, send_size);
    ASSERT_TRUE(message->type == CONNECT, result);
    ASSERT_TRUE(message->body.connect_data.version == 3u, result);
    ASSERT_TRUE(message->body.connect_data.has_clean, result);

    ASSERT_TRUE(message->body.connect_data.has_user, result);
    char *user = message->body.connect_data.user;
    uint16_t user_size = message->body.connect_data.user_size;
    ASSERT_TRUE(user_size == 4, result);
    ASSERT_TRUE(strncmp(user, "fred", user_size) == 0, result);

    ASSERT_TRUE(message->body.connect_data.has_pw, result);
    char *pw = message->body.connect_data.pw;
    uint16_t pw_size = message->body.connect_data.pw_size;
    ASSERT_TRUE(pw_size == 8, result);
    ASSERT_TRUE(strncmp(pw, "mysecret", pw_size) == 0, result);

    ASSERT_TRUE(message->body.connect_data.has_will, result);
    ASSERT_TRUE(message->body.connect_data.will_retain, result);
    ASSERT_TRUE(message->body.connect_data.will_qos == QoS2, result);
    char *will_topic = message->body.connect_data.will_topic;
    uint16_t will_topic_size = message->body.connect_data.will_topic_size;
    ASSERT_TRUE(will_topic_size == 9, result);
    ASSERT_TRUE(strncmp(will_topic, "willtopic", will_topic_size) == 0, result);
    char *will_message = message->body.connect_data.will_message;
    uint16_t will_message_size = message->body.connect_data.will_message_size;
    ASSERT_TRUE(will_message_size == 7, result);
    ASSERT_TRUE(strncmp(will_message, "willmsg", will_message_size) == 0, result);

    ASSERT_TRUE(message->body.connect_data.keep_alive == keep_alive_sec, result);
    char *name = message->body.connect_data.name;
    uint16_t name_size = message->body.connect_data.name_size;
    ASSERT_TRUE(name_size == 6, result);
    ASSERT_TRUE(strncmp(name, "MQIsdp", name_size) == 0, result);

    return result;
}

test_result_t
test_connect_message_mixed_v3()
{
    test_result_t result;
    init_result(&result);

    char buffer[BUFFER_SIZE];

    uint16_t keep_alive_sec = 30u;

    size_t send_size =
            make_connect_message(buffer, BUFFER_SIZE, "foo", NULL, "mysecret",
                                 keep_alive_sec, false, true, QoS2, false, "willtopic", "willmsg");

    ASSERT_TRUE(send_size >= 0, result);

    response_t *message = deserialize_response(buffer, send_size);
    ASSERT_TRUE(message->type == CONNECT, result);
    ASSERT_TRUE(message->body.connect_data.version == 3u, result);
    ASSERT_FALSE(message->body.connect_data.has_clean, result);

    ASSERT_FALSE(message->body.connect_data.has_user, result);

    ASSERT_TRUE(message->body.connect_data.has_pw, result);
    char *pw = message->body.connect_data.pw;
    uint16_t pw_size = message->body.connect_data.pw_size;
    ASSERT_TRUE(pw_size == 8, result);
    ASSERT_TRUE(strncmp(pw, "mysecret", pw_size) == 0, result);

    ASSERT_TRUE(message->body.connect_data.has_will, result);
    ASSERT_FALSE(message->body.connect_data.will_retain, result);
    ASSERT_TRUE(message->body.connect_data.will_qos == QoS2, result);
    char *will_topic = message->body.connect_data.will_topic;
    uint16_t will_topic_size = message->body.connect_data.will_topic_size;
    ASSERT_TRUE(will_topic_size == 9, result);
    ASSERT_TRUE(strncmp(will_topic, "willtopic", will_topic_size) == 0, result);
    char *will_message = message->body.connect_data.will_message;
    uint16_t will_message_size = message->body.connect_data.will_message_size;
    ASSERT_TRUE(will_message_size == 7, result);
    ASSERT_TRUE(strncmp(will_message, "willmsg", will_message_size) == 0, result);

    ASSERT_TRUE(message->body.connect_data.keep_alive == keep_alive_sec, result);
    char *name = message->body.connect_data.name;
    uint16_t name_size = message->body.connect_data.name_size;
    ASSERT_TRUE(name_size == 6, result);
    ASSERT_TRUE(strncmp(name, "MQIsdp", name_size) == 0, result);

    return result;
}

test_result_t
test_connect_message_will_v3()
{
    test_result_t result;
    init_result(&result);

    char buffer[BUFFER_SIZE];

    uint16_t keep_alive_sec = 30u;

    size_t send_size =
            make_connect_message(buffer, BUFFER_SIZE, "foo", NULL, NULL,
                                 keep_alive_sec, false, true, QoS2, false, "willtopic", "willmsg");

    ASSERT_TRUE(send_size >= 0, result);

    response_t *message = deserialize_response(buffer, send_size);
    ASSERT_TRUE(message->type == CONNECT, result);
    ASSERT_TRUE(message->body.connect_data.version == 3u, result);
    ASSERT_FALSE(message->body.connect_data.has_clean, result);

    ASSERT_FALSE(message->body.connect_data.has_user, result);
    ASSERT_FALSE(message->body.connect_data.has_pw, result);

    ASSERT_TRUE(message->body.connect_data.has_will, result);
    ASSERT_FALSE(message->body.connect_data.will_retain, result);
    ASSERT_TRUE(message->body.connect_data.will_qos == QoS2, result);
    char *will_topic = message->body.connect_data.will_topic;
    uint16_t will_topic_size = message->body.connect_data.will_topic_size;
    ASSERT_TRUE(will_topic_size == 9, result);
    ASSERT_TRUE(strncmp(will_topic, "willtopic", will_topic_size) == 0, result);
    char *will_message = message->body.connect_data.will_message;
    uint16_t will_message_size = message->body.connect_data.will_message_size;
    ASSERT_TRUE(will_message_size == 7, result);
    ASSERT_TRUE(strncmp(will_message, "willmsg", will_message_size) == 0, result);

    ASSERT_TRUE(message->body.connect_data.keep_alive == keep_alive_sec, result);
    char *name = message->body.connect_data.name;
    uint16_t name_size = message->body.connect_data.name_size;
    ASSERT_TRUE(name_size == 6, result);
    ASSERT_TRUE(strncmp(name, "MQIsdp", name_size) == 0, result);

    return result;
}


test_result_t
test_connack_message_v3()
{
    test_result_t result;
    init_result(&result);

    char buffer[BUFFER_SIZE];

    size_t send_size = make_connack_message(buffer, BUFFER_SIZE, Connection_Accepted);
    ASSERT_TRUE(send_size >= 0, result);

    response_t *message = deserialize_response(buffer, send_size);
    ASSERT_TRUE(message->type == CONNACK, result);
    ASSERT_TRUE(message->body.connack_data.type == Connection_Accepted, result);

    return result;
}

test_result_t
test_disconnect_message()
{
    test_result_t result;
    init_result(&result);

    char buffer[BUFFER_SIZE];

    size_t send_size = make_disconnect_message(buffer, BUFFER_SIZE);
    ASSERT_TRUE(send_size >= 0, result);

    response_t *message = deserialize_response(buffer, send_size);
    ASSERT_TRUE(message->type == DISCONNECT, result);

    return result;
}

test_result_t
test_publish_message_v3()
{
    test_result_t result;
    init_result(&result);

    char buffer[BUFFER_SIZE];

    size_t send_size =
            make_publish_message(buffer, BUFFER_SIZE, "boo", 55, QoS2, false, "hello");
    ASSERT_TRUE(send_size >= 0, result);

    response_t *message = deserialize_response(buffer, send_size);
    ASSERT_TRUE(message->type == PUBLISH, result);
    ASSERT_TRUE(message->body.publish_data.qos == QoS2, result);
    ASSERT_TRUE(message->body.publish_data.retain == false, result);
    ASSERT_TRUE(message->body.publish_data.packet_id == 55, result);
    char *topic = message->body.publish_data.topic;
    uint16_t topic_size = message->body.publish_data.topic_size;
    ASSERT_TRUE(topic_size == 3, result);
    ASSERT_TRUE(strncmp(topic, "boo", topic_size) == 0, result);
    char *content = message->body.publish_data.message;
    uint16_t content_size = message->body.publish_data.message_size;
    ASSERT_TRUE(strncmp(content, "hello", content_size) == 0, result);
    ASSERT_TRUE(content_size == 5, result);

    return result;
}

test_result_t
test_publish_message_retain_v3()
{
    test_result_t result;
    init_result(&result);

    char buffer[BUFFER_SIZE];

    size_t send_size =
            make_publish_message(buffer, BUFFER_SIZE, "boo", 55, QoS2, true, "hello");
    ASSERT_TRUE(send_size >= 0, result);

    response_t *message = deserialize_response(buffer, send_size);
    ASSERT_TRUE(message->type == PUBLISH, result);
    ASSERT_TRUE(message->body.publish_data.qos == QoS2, result);
    ASSERT_TRUE(message->body.publish_data.retain == true, result);
    ASSERT_TRUE(message->body.publish_data.packet_id == 55, result);
    char *topic = message->body.publish_data.topic;
    uint16_t topic_size = message->body.publish_data.topic_size;
    ASSERT_TRUE(topic_size == 3, result);
    ASSERT_TRUE(strncmp(topic, "boo", topic_size) == 0, result);
    char *content = message->body.publish_data.message;
    uint16_t content_size = message->body.publish_data.message_size;
    ASSERT_TRUE(strncmp(content, "hello", content_size) == 0, result);
    ASSERT_TRUE(content_size == 5, result);

    return result;
}

test_result_t
test_puback_message_v3()
{
    test_result_t result;
    init_result(&result);

    char buffer[BUFFER_SIZE];

    size_t send_size =
            make_puback_message(buffer, BUFFER_SIZE, 77);
    ASSERT_TRUE(send_size >= 0, result);

    response_t *message = deserialize_response(buffer, send_size);
    ASSERT_TRUE(message->type == PUBACK, result);
    ASSERT_TRUE(message->body.puback_data.packet_id == 77, result);

    return result;
}

test_result_t
test_subscribe_message_v3()
{
    test_result_t result;
    init_result(&result);

    char buffer[BUFFER_SIZE];

    const char *topic1 = "topic1";
    const char *topic2 = "longer/topic2";
    const char *topics[] = { topic1 , topic2 };
    QoS qoss[] = { QoS2, QoS1 };

    size_t send_size =
            make_subscribe_message(buffer, BUFFER_SIZE, 88, 2, topics, qoss);
    ASSERT_TRUE(send_size >= 0, result);

    response_t *message = deserialize_response(buffer, send_size);
    ASSERT_TRUE(message->type == SUBSCRIBE, result);
    // ODO: wrong result?(88)
    ASSERT_TRUE(message->body.subscribe_data.packet_id == 88, result);
    ASSERT_TRUE(message->body.subscribe_data.topic_count == 2, result);

    ASSERT_TRUE(message->body.subscribe_data.topic_lengths[0] == strlen(topic1), result);
    ASSERT_TRUE(strncmp(message->body.subscribe_data.topics[0], topic1, strlen(topic1)) == 0, result);
    ASSERT_TRUE(message->body.subscribe_data.qoss[0] == QoS2, result);

    ASSERT_TRUE(message->body.subscribe_data.topic_lengths[1] == strlen(topic2), result);
    ASSERT_TRUE(strncmp(message->body.subscribe_data.topics[1], topic2, strlen(topic2)) == 0, result);
    ASSERT_TRUE(message->body.subscribe_data.qoss[1] == QoS1, result);


    return result;
}

test_result_t
test_unsubscribe_message_v3()
{
    test_result_t result;
    init_result(&result);

    char buffer[BUFFER_SIZE];

    const char *topic1 = "topic1";
    const char *topic2 = "longer/topic2";
    const char *topics[] = { topic1 , topic2 };

    size_t send_size =
            make_unsubscribe_message(buffer, BUFFER_SIZE, 88, 2, topics);
    ASSERT_TRUE(send_size >= 0, result);

    response_t *message = deserialize_response(buffer, send_size);
    ASSERT_TRUE(message->type == UNSUBSCRIBE, result);
    ASSERT_TRUE(message->body.unsubscribe_data.packet_id == 88, result);
    ASSERT_TRUE(message->body.unsubscribe_data.topic_count == 2, result);

    ASSERT_TRUE(message->body.unsubscribe_data.topic_lengths[0] == strlen(topic1), result);
    ASSERT_TRUE(strncmp(message->body.unsubscribe_data.topics[0], topic1, strlen(topic1)) == 0, result);

    ASSERT_TRUE(message->body.unsubscribe_data.topic_lengths[1] == strlen(topic2), result);
    ASSERT_TRUE(strncmp(message->body.unsubscribe_data.topics[1], topic2, strlen(topic2)) == 0, result);

    return result;
}

test_result_t
test_suback_message_v3()
{
    test_result_t result;
    init_result(&result);

    char buffer[BUFFER_SIZE];

    bool success[] = { true };
    QoS qoss[] = { QoS2 };

    size_t send_size =
            make_suback_message(buffer, BUFFER_SIZE, 99, 1, success, qoss);
    ASSERT_TRUE(send_size >= 0, result);

    response_t *message = deserialize_response(buffer, send_size);
    ASSERT_TRUE(message->type == SUBACK, result);
    ASSERT_TRUE(message->body.suback_data.packet_id == 99, result);
    ASSERT_TRUE(message->body.suback_data.topic_count == 1, result);
    ASSERT_TRUE(message->body.suback_data.success[0], result);
    ASSERT_TRUE(message->body.suback_data.qoss[0] == QoS2, result);

    return result;
}

test_result_t
test_suback_message_mixed_v3()
{
    test_result_t result;
    init_result(&result);

    char buffer[BUFFER_SIZE];

    bool success[] = { true, false, true, true };
    QoS qoss[] = { QoS2, QoS0, QoS1, QoS0 };

    size_t send_size =
            make_suback_message(buffer, BUFFER_SIZE, 99, 4, success, qoss);
    ASSERT_TRUE(send_size >= 0, result);

    response_t *message = deserialize_response(buffer, send_size);
    ASSERT_TRUE(message->type == SUBACK, result);
    ASSERT_TRUE(message->body.suback_data.packet_id == 99, result);
    ASSERT_TRUE(message->body.suback_data.topic_count == 4, result);

    ASSERT_TRUE(message->body.suback_data.success[0], result);
    ASSERT_TRUE(message->body.suback_data.qoss[0] == QoS2, result);

    ASSERT_TRUE(message->body.suback_data.success[1] == false, result);

    ASSERT_TRUE(message->body.suback_data.success[2], result);
    ASSERT_TRUE(message->body.suback_data.qoss[2] == QoS1, result);

    ASSERT_TRUE(message->body.suback_data.success[3], result);
    ASSERT_TRUE(message->body.suback_data.qoss[3] == QoS0, result);

    return result;
}

