#include "messages.h"
#include "messages_internal.h"
#include "smqttc_internal.h"
#include "smqttc.h"

#include "test_macros.h"

test_result_t
test_no_auth_v3()
{
    test_result_t result;
    init_result(&result);

    smqtt_client_t *client1 = NULL;
    smqtt_status_t status =
            smqtt_connect_internal(REMOTE_SERVER, get_server_host(), 1883,
                                  "test_client_1", NULL, NULL ,30u, true,
                                  false, QoS0, false, NULL, NULL,
                                  &client1);

    ASSERT_TRUE(status == SMQTT_NOT_AUTHORIZED, result)
    ASSERT_TRUE(client1 == NULL, result)

    return result;
}

test_result_t
test_bad_auth_v3()
{
    test_result_t result;
    init_result(&result);

    smqtt_client_t *client1 = NULL;
    smqtt_status_t status =
            smqtt_connect_internal(REMOTE_SERVER, get_server_host(), 1883,
                                  "test_client_1", "Fred", "Secret" ,30u, true,
                                  false, QoS0, false, NULL, NULL,
                                  &client1);

    ASSERT_TRUE(status == SMQTT_NOT_AUTHORIZED, result)
    ASSERT_TRUE(client1 == NULL, result)

    return result;
}

test_result_t
test_basic_v3_qos0()
{
    test_result_t result;
    init_result(&result);

    static const char *topics[] = {"test_topic", NULL};
    static QoS qoss[] = { QoS0 };

    smqtt_client_t *client1 = NULL;
    smqtt_status_t status =
            smqtt_connect_internal(REMOTE_SERVER, get_server_host(), 1883,
                                  "test_client_1", "user1", "password1",30u, true,
                                  false, QoS0, false, NULL, NULL,
                                  &client1);

    ASSERT_TRUE(status == SMQTT_OK, result)

    smqtt_client_t *client2 = NULL;
    status =
            smqtt_connect_internal(REMOTE_SERVER, get_server_host(), 1883,
                                  "test_client_2", "user1", "password1",30u, true,
                                  false, QoS0, false, NULL, NULL,
                                  &client2);

    ASSERT_TRUE(status == SMQTT_OK, result)

    status = smqtt_subscribe(client2, topics, qoss);
    ASSERT_TRUE(status == SMQTT_OK, result)

    static char *message1 = "hello1";
    status = smqtt_publish(client1, topics[0], message1, QoS0, false);
    ASSERT_TRUE(status == SMQTT_OK, result)

    uint16_t packet_id = 0u; // so we can check it
    char *message;
    uint16_t message_size;
    status = smqtt_get_next_message(client2, &packet_id, &message, &message_size);
    ASSERT_TRUE(status == 0, result)
    ASSERT_TRUE(packet_id == 0, result) // not set
    ASSERT_TRUE(message_size == strlen(message1), result)
    ASSERT_TRUE(strncmp(message1, message, message_size) == 0, result)

    smqtt_disconnect(client1);

    smqtt_disconnect(client2);


    return result;
}

test_result_t
test_message_backlog_v3_qos0()
{
    test_result_t result;
    init_result(&result);

    static const char *topics[] = {"test_topic", NULL};
    static QoS qoss[] = { QoS0 };

    smqtt_client_t *client1 = NULL;
    smqtt_status_t status = smqtt_connect_internal(REMOTE_SERVER, get_server_host(), 1883,
                                                   "test_client_1", "user1", "password1",30u, true,
                                                   false, QoS0, false, NULL, NULL,
                                                   &client1);
    ASSERT_TRUE(status == SMQTT_OK, result)

    smqtt_client_t *client2 = NULL;
    status = smqtt_connect_internal(REMOTE_SERVER, get_server_host(), 1883,
                                                   "test_client_2", "user1", "password1",30u, true,
                                                   false, QoS0, false, NULL, NULL,
                                                   &client2);
    ASSERT_TRUE(status == SMQTT_OK, result)

    status = smqtt_subscribe(client2, topics, qoss);
    ASSERT_TRUE(status == SMQTT_OK, result)

    static char *message1 = "hello1";
    status = smqtt_publish(client1, topics[0], message1, QoS0, false);
    ASSERT_TRUE(status == SMQTT_OK, result)

    static char *message2 = "longhello2";
    status = smqtt_publish(client1, topics[0], message2, QoS0, false);
    ASSERT_TRUE(status == SMQTT_OK, result)

    uint16_t packet_id = 0u; // so we can check it
    char *message;
    uint16_t message_size;
    status = smqtt_get_next_message(client2, &packet_id, &message, &message_size);
    ASSERT_TRUE(status == SMQTT_OK, result)
    ASSERT_TRUE(packet_id == 0, result) // not set
    ASSERT_TRUE(message_size == strlen(message1), result)
    ASSERT_TRUE(strncmp(message1, message, message_size) == 0, result)

    packet_id = 0u; // so we can check it
    message = NULL;
    status = smqtt_get_next_message(client2, &packet_id, &message, &message_size);
    ASSERT_TRUE(status == SMQTT_OK, result)
    ASSERT_TRUE(packet_id == 0, result) // not set
    ASSERT_TRUE(message_size == strlen(message2), result)
    ASSERT_TRUE(strncmp(message2, message, message_size) == 0, result)

    smqtt_disconnect(client1);

    smqtt_disconnect(client2);


    return result;
}

test_result_t
test_basic_v3_qos1()
{
    test_result_t result;
    init_result(&result);

    static const char *topics[] = {"test_topic", NULL};
    static QoS qoss[] = { QoS1 };

    smqtt_client_t *client1 = NULL;
    smqtt_status_t status = smqtt_connect_internal(REMOTE_SERVER, get_server_host(), 1883,
                                                   "test_client_1", "user1", "password1",30u, true,
                                                   false, QoS0, false, NULL, NULL,
                                                   &client1);
    ASSERT_TRUE(status == SMQTT_OK, result)

    smqtt_client_t *client2 = NULL;
    status = smqtt_connect_internal(REMOTE_SERVER, get_server_host(), 1883,
                                                   "test_client_2", "user1", "password1",30u, true,
                                                   false, QoS0, false, NULL, NULL,
                                                   &client2);
    ASSERT_TRUE(status == SMQTT_OK, result)

    status = smqtt_subscribe(client2, topics, qoss);
    ASSERT_TRUE(status == SMQTT_OK, result)

    static char *message1 = "hello1";
    status = smqtt_publish(client1, topics[0], message1, QoS1, false);
    ASSERT_TRUE(status == SMQTT_OK, result)

    uint16_t packet_id = 0u; // so we can check it
    char *message;
    uint16_t message_size;
    status = smqtt_get_next_message(client2, &packet_id, &message, &message_size);
    ASSERT_TRUE(status == SMQTT_OK, result)
    ASSERT_TRUE(packet_id == 1, result)
    ASSERT_TRUE(message_size == strlen(message1), result)
    ASSERT_TRUE(strncmp(message1, message, message_size) == 0, result)

    smqtt_disconnect(client1);

    smqtt_disconnect(client2);


    return result;
}

test_result_t
test_basic_v3_qos2_qos2()
{
    test_result_t result;
    init_result(&result);

    static const char *topics[] = {"test_topic", NULL};
    static QoS qoss[] = { QoS2 };

    smqtt_client_t *client1 = NULL;
    smqtt_status_t status = smqtt_connect_internal(REMOTE_SERVER, get_server_host(), 1883,
                                                   "test_client_1", "user1", "password1",30u, true,
                                                   false, QoS0, false, NULL, NULL,
                                                   &client1);
    ASSERT_TRUE(status == SMQTT_OK, result)

    smqtt_client_t *client2 = NULL;
    status = smqtt_connect_internal(REMOTE_SERVER, get_server_host(), 1883,
                                                   "test_client_2", "user1", "password1",30u, true,
                                                   false, QoS0, false, NULL, NULL,
                                                   &client2);
    ASSERT_TRUE(status == SMQTT_OK, result)

    status = smqtt_subscribe(client2, topics, qoss);
    ASSERT_TRUE(status == SMQTT_OK, result)

    static char *message1 = "hello1";
    status = smqtt_publish(client1, topics[0], message1, QoS2, false);
    ASSERT_TRUE(status == SMQTT_OK, result)

    uint16_t packet_id = 0u; // so we can check it
    char *message;
    uint16_t message_size;
    status = smqtt_get_next_message(client2, &packet_id, &message, &message_size);
    ASSERT_TRUE(status == SMQTT_OK, result)
    ASSERT_TRUE(packet_id == 1, result)
    ASSERT_TRUE(message_size == strlen(message1), result)
    ASSERT_TRUE(strncmp(message1, message, message_size) == 0, result)

    static char *message2 = "longhello2";
    status = smqtt_publish(client1, topics[0], message2, QoS2, false);
    ASSERT_TRUE(status == SMQTT_OK, result)

    packet_id = 0u;
    status = smqtt_get_next_message(client2, &packet_id, &message, &message_size);
    ASSERT_TRUE(status == SMQTT_OK, result)
    ASSERT_TRUE(packet_id == 2, result)
    ASSERT_TRUE(message_size == strlen(message2), result)
    ASSERT_TRUE(strncmp(message2, message, message_size) == 0, result)

    smqtt_disconnect(client1);

    smqtt_disconnect(client2);


    return result;
}

test_result_t
test_ping_v3()
{
    test_result_t result;
    init_result(&result);

    smqtt_client_t *client1 = NULL;
    smqtt_status_t status = smqtt_connect_internal(REMOTE_SERVER, get_server_host(), 1883,
                                                   "test_client_1", "user1", "password1",30u, true,
                                                   false, QoS0, false, NULL, NULL,
                                                   &client1);

    ASSERT_TRUE(status == SMQTT_OK, result)

    status = smqtt_ping(client1);
    ASSERT_TRUE(status == SMQTT_OK, result)

    status = smqtt_ping(client1);
    ASSERT_TRUE(status == SMQTT_OK, result)

    smqtt_disconnect(client1);

    return result;
}

test_result_t
test_unsubscribe_v3()
{
    test_result_t result;
    init_result(&result);

    smqtt_client_t *client1 = NULL;
    smqtt_status_t status = smqtt_connect_internal(REMOTE_SERVER, get_server_host(), 1883,
                                                   "test_client_1", "user1", "password1",30u, true,
                                                   false, QoS0, false, NULL, NULL,
                                                   &client1);
    ASSERT_TRUE(status == SMQTT_OK, result)

    const char *topic1 = "topic1";
    const char *topic2 = "longer/topic2";
    const char *topics[] = { topic1 , topic2, NULL };
    static QoS qoss[] = { QoS2, QoS2 };


    status = smqtt_subscribe(client1, topics, qoss);
    ASSERT_TRUE(status == 0, result)

    status = smqtt_unsubscribe(client1, topics);
    ASSERT_TRUE(status == 0, result)

    smqtt_disconnect(client1);

    return result;
}

test_result_t
test_pubsub_retain_v3()
{
    test_result_t result;
    init_result(&result);

    static const char *topics[] = {"test_topic", NULL};
    static QoS qoss[] = { QoS2 };

    smqtt_client_t *client1 = NULL;
    smqtt_status_t status = smqtt_connect_internal(REMOTE_SERVER, get_server_host(), 1883,
                                                   "test_client_1", "user1", "password1",30u, true,
                                                   false, QoS0, false, NULL, NULL,
                                                   &client1);
    ASSERT_TRUE(status == SMQTT_OK, result)

    static char *message1 = "hello1";
    status = smqtt_publish(client1, topics[0], message1, QoS2, true);
    ASSERT_TRUE(status == 0, result)

    smqtt_client_t *client2 = NULL;
    status = smqtt_connect_internal(REMOTE_SERVER, get_server_host(), 1883,
                                                   "test_client_2", "user1", "password1",30u, true,
                                                   false, QoS0, false, NULL, NULL,
                                                   &client2);
    ASSERT_TRUE(status == SMQTT_OK, result)

    status = smqtt_subscribe(client2, topics, qoss);
    ASSERT_TRUE(status == SMQTT_OK, result)

    uint16_t packet_id = 0u; // so we can check it
    char *message;
    uint16_t message_size;
    status = smqtt_get_next_message(client2, &packet_id, &message, &message_size);
    ASSERT_TRUE(status == SMQTT_OK, result)
    ASSERT_TRUE(packet_id == 1, result)
    ASSERT_TRUE(message_size == strlen(message1), result)
    ASSERT_TRUE(strncmp(message1, message, message_size) == 0, result)

    // clean up retained message by publishing an empty message
    status = smqtt_publish(client1, topics[0], NULL, QoS2, true);
    ASSERT_TRUE(status == 0, result)

    smqtt_disconnect(client1);

    smqtt_disconnect(client2);


    return result;
}

test_result_t
test_pubsub_will_v3()
{
    test_result_t result;
    init_result(&result);

    static const char *topics[] = {"willtopic", NULL};
    static QoS qoss[] = { QoS2 };

    smqtt_client_t *client1 = NULL;
    smqtt_status_t status = smqtt_connect_internal(REMOTE_SERVER, get_server_host(), 1883,
                                                   "will_client", "user1", "password1",30u, true,
                                                   true, QoS2, false,
                                                   "willtopic", "willmessage",
                                                   &client1);
    ASSERT_TRUE(status == SMQTT_OK, result)

    // clean up any retained message by publishing an empty message
    status = smqtt_publish(client1, topics[0], NULL, QoS2, true);
    ASSERT_TRUE(status == 0, result);

    smqtt_client_t *client2 = NULL;
    status = smqtt_connect_internal(REMOTE_SERVER, get_server_host(), 1883,
                                                   "test_client_2", "user1", "password1",30u, true,
                                                   false, QoS0, false, NULL, NULL,
                                                   &client2);

    status = smqtt_subscribe(client2, topics, qoss);
    ASSERT_TRUE(status == SMQTT_OK, result)

    // simulate sudden disappearance of first client
    mqtt_drop(client1);

    // now the other client should get a last will & testament BUT it
    // won't have been retained

    uint16_t packet_id = 0u; // so we can check it
    char *message;
    uint16_t message_size;
    status = smqtt_get_next_message(client2, &packet_id, &message, &message_size);
    ASSERT_TRUE(status == 0, result)
    ASSERT_TRUE(packet_id == 1, result) // not set
    ASSERT_TRUE(message_size == strlen("willmessage"), result)
    ASSERT_TRUE(strncmp("willmessage", message, message_size) == 0, result)

    smqtt_disconnect(client2);


    return result;
}

test_result_t
test_pubsub_will_retained_v3()
{
    test_result_t result;
    init_result(&result);

    static const char *topics[] = {"willtopic", NULL};
    static QoS qoss[] = { QoS2 };

    smqtt_client_t *client1 = NULL;
    smqtt_status_t stat = smqtt_connect_internal(REMOTE_SERVER, get_server_host(), 1883,
                                                   "will_client", "user1", "password1",30u, true,
                                                   true, QoS2, true,
                                                   "willtopic", "willmessage",
                                                   &client1);

    // clean up any retained message by publishing an empty message
    int status = smqtt_publish(client1, topics[0], NULL, QoS2, true);
    ASSERT_TRUE(status == 0, result)


    smqtt_client_t *client2 = NULL;
    stat = smqtt_connect_internal(REMOTE_SERVER, get_server_host(), 1883,
                                                   "test_client_2", "user1", "password1",30u, true,
                                                   false, QoS0, false, NULL, NULL,
                                                   &client2);

    // simulate sudden disappearance of first client
    mqtt_drop(client1);

    status = smqtt_subscribe(client2, topics, qoss);
    ASSERT_TRUE(status == 0, result)

    // now this client should get a last will & testament as it was retained

    uint16_t packet_id = 0u; // so we can check it
    char *message;
    uint16_t message_size;
    status = smqtt_get_next_message(client2, &packet_id, &message, &message_size);
    ASSERT_TRUE(status == 0, result)
    ASSERT_TRUE(packet_id == 1, result) // not set
    ASSERT_TRUE(message_size == strlen("willmessage"), result)
    ASSERT_TRUE(strncmp("willmessage", message, message_size) == 0, result)

    smqtt_disconnect(client2);


    return result;
}

