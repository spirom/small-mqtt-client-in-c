#include <sys/select.h>
#include <malloc.h>

#include "messages.h"
#include "messages_internal.h"
#include "smqttc_mt_internal.h"
#include "smqttc_mt.h"
#include "server.h"

#include "test_macros.h"

static void test_sleep(uint64_t msec)
{
    struct timeval tv;
    tv.tv_sec = msec / 1000;
    tv.tv_usec = (msec % 1000) * 1000;
    select(0, NULL, NULL, NULL, &tv);
}

typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t cond;
    bool done;
    union {
        struct {
            uint16_t topic_count;
            bool success[MAX_TOPICS];
            QoS qoss[MAX_TOPICS];
        } subscription_response;
        struct {
            char *topic;
            char *msg;
            QoS qos;
            bool retain;
        } message_response;
    };
} cb_state_t;

cb_state_t *create_cb_state()
{
    cb_state_t *state = (cb_state_t *)malloc(sizeof(cb_state_t));
    pthread_mutex_init(&state->lock, NULL);
    pthread_cond_init(&state->cond, NULL);
    state->done = false;
    return state;
}

void notify_cb_state(cb_state_t *state)
{
    pthread_mutex_lock(&state->lock);
    // wait until the single slot is empty
    while (state->done)
        pthread_cond_wait(&state->cond, &state->lock);
    state->done = true;
    pthread_cond_broadcast(&state->cond);
    pthread_mutex_unlock(&state->lock);
}

void request_cb(bool returned, void *context)
{
    printf("called back\n");
    notify_cb_state((cb_state_t *) context);
}

void subscribe_cb(
        bool returned,
        uint16_t packet_id,
        uint16_t topic_count,
        const bool success[],
        const QoS qoss[],
        void *context)
{
    printf("called back [subscribe]\n");
    cb_state_t *cb_state = (cb_state_t *) context;
    cb_state->subscription_response.topic_count = topic_count;
    for (uint16_t i = 0; i < topic_count; i++) {
        cb_state->subscription_response.success[i] = success[i];
        cb_state->subscription_response.qoss[i] = qoss[i];
    }
    notify_cb_state((cb_state_t *) context);
}

void message_cb(
        char *topic,
        char *msg,
        QoS qos,
        bool retain,
        void *context)
{
    printf("called back [message]\n");
    cb_state_t *state = (cb_state_t *) context;
    pthread_mutex_lock(&state->lock);
    // wait until the single slot is available
    while (state->done)
        pthread_cond_wait(&state->cond, &state->lock);
    // fill the slot and signal
    state->message_response.topic = topic;
    state->message_response.msg = msg;
    state->message_response.qos = qos;
    state->message_response.retain = retain;
    state->done = true;
    pthread_cond_broadcast(&state->cond);
    pthread_mutex_unlock(&state->lock);
}

bool wait_for_cb(cb_state_t *state)
{
    pthread_mutex_lock(&state->lock);
    while (!state->done)
        pthread_cond_wait(&state->cond, &state->lock);
    pthread_mutex_unlock(&state->lock);
    return true;
}

void release_cb(cb_state_t *state)
{
    // assumption: slot is full and it's ours to consume
    pthread_mutex_lock(&state->lock);
    state->done = false;
    pthread_cond_broadcast(&state->cond);
    pthread_mutex_unlock(&state->lock);
}


test_result_t
test_connect_disconnect_v3()
{
    test_result_t result;
    init_result(&result);

    smqtt_mt_client_t *client1 = NULL;
    smqtt_mt_status_t status =
            smqtt_mt_connect_internal(REMOTE_SERVER, "127.0.0.1", 1883,
                                   "test_client_1", "user1", "password1",30u, true,
                                   false, QoS0, false, NULL, NULL, NULL, NULL,
                                   &client1);

    ASSERT_TRUE(status == SMQTT_MT_OK, result);
    ASSERT_TRUE(client1 != NULL, result);

    status = smqtt_mt_disconnect(client1);
    ASSERT_TRUE(status == SMQTT_MT_OK, result);

    return result;
}

test_result_t
test_ping_v3()
{
    test_result_t result;
    init_result(&result);

    smqtt_mt_client_t *client1 = NULL;
    smqtt_mt_status_t status =
            smqtt_mt_connect_internal(REMOTE_SERVER, "127.0.0.1", 1883,
                                      "test_client_1", "user1", "password1",30u, true,
                                      false, QoS0, false, NULL, NULL, NULL, NULL,
                                      &client1);

    ASSERT_TRUE(status == SMQTT_MT_OK, result);
    ASSERT_TRUE(client1 != NULL, result);

    cb_state_t *state1 = create_cb_state();
    cb_state_t *state2 = create_cb_state();

    status = smqtt_mt_ping(client1, 500, &request_cb, state1);
    ASSERT_TRUE(status == SMQTT_MT_OK, result);

    status = smqtt_mt_ping(client1, 500, &request_cb, state2);
    ASSERT_TRUE(status == SMQTT_MT_OK, result);

    bool got_callback_before_timeout = wait_for_cb(state1);
    ASSERT_TRUE(got_callback_before_timeout, result);

    got_callback_before_timeout = wait_for_cb(state2);
    ASSERT_TRUE(got_callback_before_timeout, result);

    status = smqtt_mt_disconnect(client1);
    ASSERT_TRUE(status == SMQTT_MT_OK, result);

    return result;
}

test_result_t
test_publish_qos0_v3()
{
    test_result_t result;
    init_result(&result);

    smqtt_mt_client_t *client1 = NULL;
    smqtt_mt_status_t status =
            smqtt_mt_connect_internal(REMOTE_SERVER, "127.0.0.1", 1883,
                                      "test_client_1", "user1", "password1",30u, true,
                                      false, QoS0, false, NULL, NULL, NULL, NULL,
                                      &client1);

    ASSERT_TRUE(status == SMQTT_MT_OK, result);
    ASSERT_TRUE(client1 != NULL, result);

    status = smqtt_mt_publish(
            client1, "test_topic", "hello", QoS0, false, NULL, NULL);
    ASSERT_TRUE(status == SMQTT_MT_OK, result);

    test_sleep(100); // give it time to send

    status = smqtt_mt_disconnect(client1);
    ASSERT_TRUE(status == SMQTT_MT_OK, result);

    return result;
}

test_result_t
test_publish_qos1_v3()
{
    test_result_t result;
    init_result(&result);

    smqtt_mt_client_t *client1 = NULL;
    smqtt_mt_status_t status =
            smqtt_mt_connect_internal(REMOTE_SERVER, "127.0.0.1", 1883,
                                      "test_client_1", "user1", "password1",30u, true,
                                      false, QoS0, false, NULL, NULL, NULL, NULL,
                                      &client1);

    cb_state_t *state1 = create_cb_state();
    cb_state_t *state2 = create_cb_state();

    ASSERT_TRUE(status == SMQTT_MT_OK, result);
    ASSERT_TRUE(client1 != NULL, result);

    status = smqtt_mt_publish(
            client1, "test_topic", "hello", QoS1, false, &request_cb, state1);
    ASSERT_TRUE(status == SMQTT_MT_OK, result);

    status = smqtt_mt_publish(
            client1, "test_topic", "hello", QoS1, false, &request_cb, state2);
    ASSERT_TRUE(status == SMQTT_MT_OK, result);

    bool got_callback_before_timeout = wait_for_cb(state1);
    ASSERT_TRUE(got_callback_before_timeout, result);

    got_callback_before_timeout = wait_for_cb(state2);
    ASSERT_TRUE(got_callback_before_timeout, result);

    status = smqtt_mt_disconnect(client1);
    ASSERT_TRUE(status == SMQTT_MT_OK, result);

    return result;
}

test_result_t
test_publish_qos2_v3()
{
    test_result_t result;
    init_result(&result);

    smqtt_mt_client_t *client1 = NULL;
    smqtt_mt_status_t status =
            smqtt_mt_connect_internal(REMOTE_SERVER, "127.0.0.1", 1883,
                                      "test_client_1", "user1", "password1",30u, true,
                                      false, QoS0, false, NULL, NULL, NULL, NULL,
                                      &client1);

    cb_state_t *state1 = create_cb_state();
    cb_state_t *state2 = create_cb_state();

    ASSERT_TRUE(status == SMQTT_MT_OK, result);
    ASSERT_TRUE(client1 != NULL, result);

    status = smqtt_mt_publish(
            client1, "test_topic", "hello", QoS2, false, &request_cb, state1);
    ASSERT_TRUE(status == SMQTT_MT_OK, result);

    status = smqtt_mt_publish(
            client1, "test_topic", "hello", QoS2, false, &request_cb, state2);
    ASSERT_TRUE(status == SMQTT_MT_OK, result);

    bool got_callback_before_timeout = wait_for_cb(state1);
    ASSERT_TRUE(got_callback_before_timeout, result);

    got_callback_before_timeout = wait_for_cb(state2);
    ASSERT_TRUE(got_callback_before_timeout, result);

    status = smqtt_mt_disconnect(client1);
    ASSERT_TRUE(status == SMQTT_MT_OK, result);

    return result;
}

test_result_t
test_subscribe_simple_v3()
{
    test_result_t result;
    init_result(&result);

    smqtt_mt_client_t *client1 = NULL;
    smqtt_mt_status_t status =
            smqtt_mt_connect_internal(REMOTE_SERVER, "127.0.0.1", 1883,
                                      "test_client_1", "user1", "password1",30u, true,
                                      false, QoS0, false, NULL, NULL, NULL, NULL,
                                      &client1);

    cb_state_t *state1 = create_cb_state();

    ASSERT_TRUE(status == SMQTT_MT_OK, result);
    ASSERT_TRUE(client1 != NULL, result);

    const char *topics[] = { "test_topic" };
    QoS qoss[] = { QoS1 };
    status = smqtt_mt_subscribe(
            client1, 1, topics, qoss, subscribe_cb, state1);
    ASSERT_TRUE(status == SMQTT_MT_OK, result);

    bool got_callback_before_timeout = wait_for_cb(state1);
    ASSERT_TRUE(got_callback_before_timeout, result);
    ASSERT_TRUE(state1->subscription_response.topic_count == 1, result);
    ASSERT_TRUE(state1->subscription_response.success[0], result);
    ASSERT_TRUE(state1->subscription_response.qoss[0] == QoS1, result);


    status = smqtt_mt_disconnect(client1);
    ASSERT_TRUE(status == SMQTT_MT_OK, result);

    return result;
}

test_result_t
test_subscribe_multiple_v3()
{
    test_result_t result;
    init_result(&result);

    smqtt_mt_client_t *client1 = NULL;
    smqtt_mt_status_t status =
            smqtt_mt_connect_internal(REMOTE_SERVER, "127.0.0.1", 1883,
                                      "test_client_1", "user1", "password1",30u, true,
                                      false, QoS0, false, NULL, NULL, NULL, NULL,
                                      &client1);

    cb_state_t *state1 = create_cb_state();

    ASSERT_TRUE(status == SMQTT_MT_OK, result);
    ASSERT_TRUE(client1 != NULL, result);

    const char *topics[] = { "test_topic_1", "test_topic_2", "test_topic_3" };
    QoS qoss[] = { QoS1, QoS2, QoS0 };
    status = smqtt_mt_subscribe(
            client1, 3, topics, qoss, subscribe_cb, state1);
    ASSERT_TRUE(status == SMQTT_MT_OK, result);

    bool got_callback_before_timeout = wait_for_cb(state1);
    ASSERT_TRUE(got_callback_before_timeout, result);
    ASSERT_TRUE(state1->subscription_response.topic_count == 3, result);
    ASSERT_TRUE(state1->subscription_response.success[0], result);
    ASSERT_TRUE(state1->subscription_response.qoss[0] == QoS1, result);
    ASSERT_TRUE(state1->subscription_response.success[1], result);
    ASSERT_TRUE(state1->subscription_response.qoss[1] == QoS2, result);
    ASSERT_TRUE(state1->subscription_response.success[2], result);
    ASSERT_TRUE(state1->subscription_response.qoss[2] == QoS0, result);

    status = smqtt_mt_disconnect(client1);
    ASSERT_TRUE(status == SMQTT_MT_OK, result);

    return result;
}

test_result_t
test_pub_sub_qos0_v3()
{
    test_result_t result;
    init_result(&result);

    smqtt_mt_client_t *publisher = NULL;
    smqtt_mt_status_t status =
            smqtt_mt_connect_internal(REMOTE_SERVER, "127.0.0.1", 1883,
                                      "pub_client_1", "user1", "password1",30u, true,
                                      false, QoS0, false, NULL, NULL, NULL, NULL,
                                      &publisher);
    ASSERT_TRUE(status == SMQTT_MT_OK, result);
    ASSERT_TRUE(publisher != NULL, result);

    cb_state_t *sub_state = create_cb_state();
    cb_state_t *sub_msg_state = create_cb_state();

    smqtt_mt_client_t *subscriber = NULL;
    status =
            smqtt_mt_connect_internal(REMOTE_SERVER, "127.0.0.1", 1883,
                                      "sub_client_1", "user1", "password1",30u, true,
                                      false, QoS0, false, NULL, NULL,
                                      message_cb, sub_msg_state,
                                      &subscriber);

    ASSERT_TRUE(status == SMQTT_MT_OK, result);
    ASSERT_TRUE(subscriber != NULL, result);

    // subscribe

    const char *topics[] = { "test_topic_1" };
    QoS qoss[] = { QoS0 };
    status = smqtt_mt_subscribe(
            subscriber, 1, topics, qoss,
            subscribe_cb, sub_state);
    ASSERT_TRUE(status == SMQTT_MT_OK, result);

    bool got_callback_before_timeout = wait_for_cb(sub_state);
    ASSERT_TRUE(got_callback_before_timeout, result);
    ASSERT_TRUE(sub_state->subscription_response.topic_count == 1, result);
    ASSERT_TRUE(sub_state->subscription_response.success[0], result);

    // publish something

    status = smqtt_mt_publish(
            publisher, topics[0], "hello", QoS0, false, NULL, NULL);
    ASSERT_TRUE(status == SMQTT_MT_OK, result);

    // check for receipt
    got_callback_before_timeout = wait_for_cb(sub_msg_state);
    ASSERT_TRUE(got_callback_before_timeout, result);
    ASSERT_TRUE(!strcmp(sub_msg_state->message_response.topic, topics[0]), result);
    ASSERT_TRUE(!strcmp(sub_msg_state->message_response.msg, "hello"), result);
    ASSERT_TRUE(sub_msg_state->message_response.qos == QoS0, result);
    ASSERT_TRUE(sub_msg_state->message_response.retain == false, result);
    release_cb(sub_msg_state);

    // disconnect

    status = smqtt_mt_disconnect(publisher);
    ASSERT_TRUE(status == SMQTT_MT_OK, result);

    status = smqtt_mt_disconnect(subscriber);
    ASSERT_TRUE(status == SMQTT_MT_OK, result);

    return result;
}

test_result_t
test_pub_sub_qos1_v3()
{
    test_result_t result;
    init_result(&result);

    smqtt_mt_client_t *publisher = NULL;
    smqtt_mt_status_t status =
            smqtt_mt_connect_internal(REMOTE_SERVER, "127.0.0.1", 1883,
                                      "pub_client_1", "user1", "password1",30u, true,
                                      false, QoS0, false, NULL, NULL, NULL, NULL,
                                      &publisher);
    ASSERT_TRUE(status == SMQTT_MT_OK, result);
    ASSERT_TRUE(publisher != NULL, result);

    cb_state_t *pub_state = create_cb_state();
    cb_state_t *sub_state = create_cb_state();
    cb_state_t *sub_msg_state = create_cb_state();

    smqtt_mt_client_t *subscriber = NULL;
    status =
            smqtt_mt_connect_internal(REMOTE_SERVER, "127.0.0.1", 1883,
                                      "sub_client_1", "user1", "password1",30u, true,
                                      false, QoS0, false, NULL, NULL,
                                      message_cb, sub_msg_state,
                                      &subscriber);

    ASSERT_TRUE(status == SMQTT_MT_OK, result);
    ASSERT_TRUE(subscriber != NULL, result);

    // subscribe

    const char *topics[] = { "test_topic_1" };
    QoS qoss[] = { QoS1 };
    status = smqtt_mt_subscribe(
            subscriber, 1, topics, qoss,
            subscribe_cb, sub_state);
    ASSERT_TRUE(status == SMQTT_MT_OK, result);

    bool got_callback_before_timeout = wait_for_cb(sub_state);
    ASSERT_TRUE(got_callback_before_timeout, result);
    ASSERT_TRUE(sub_state->subscription_response.topic_count == 1, result);
    ASSERT_TRUE(sub_state->subscription_response.success[0], result);

    // publish something

    status = smqtt_mt_publish(
            publisher, topics[0], "hello", QoS1, false, &request_cb, pub_state);
    ASSERT_TRUE(status == SMQTT_MT_OK, result);

    got_callback_before_timeout = wait_for_cb(pub_state);
    ASSERT_TRUE(got_callback_before_timeout, result);

    // check for receipt
    got_callback_before_timeout = wait_for_cb(sub_msg_state);
    ASSERT_TRUE(got_callback_before_timeout, result);
    ASSERT_TRUE(!strcmp(sub_msg_state->message_response.topic, topics[0]), result);
    ASSERT_TRUE(!strcmp(sub_msg_state->message_response.msg, "hello"), result);
    ASSERT_TRUE(sub_msg_state->message_response.qos == QoS1, result);
    ASSERT_TRUE(sub_msg_state->message_response.retain == false, result);
    release_cb(sub_msg_state);

    // disconnect

    status = smqtt_mt_disconnect(publisher);
    ASSERT_TRUE(status == SMQTT_MT_OK, result);

    status = smqtt_mt_disconnect(subscriber);
    ASSERT_TRUE(status == SMQTT_MT_OK, result);

    return result;
}

test_result_t
test_pub_sub_qos2_v3()
{
    test_result_t result;
    init_result(&result);

    smqtt_mt_client_t *publisher = NULL;
    smqtt_mt_status_t status =
            smqtt_mt_connect_internal(REMOTE_SERVER, "127.0.0.1", 1883,
                                      "pub_client_1", "user1", "password1",30u, true,
                                      false, QoS0, false, NULL, NULL, NULL, NULL,
                                      &publisher);
    ASSERT_TRUE(status == SMQTT_MT_OK, result);
    ASSERT_TRUE(publisher != NULL, result);

    cb_state_t *pub_state = create_cb_state();
    cb_state_t *sub_state = create_cb_state();
    cb_state_t *sub_msg_state = create_cb_state();

    smqtt_mt_client_t *subscriber = NULL;
    status =
            smqtt_mt_connect_internal(REMOTE_SERVER, "127.0.0.1", 1883,
                                      "sub_client_1", "user1", "password1",30u, true,
                                      false, QoS0, false, NULL, NULL,
                                      message_cb, sub_msg_state,
                                      &subscriber);

    ASSERT_TRUE(status == SMQTT_MT_OK, result);
    ASSERT_TRUE(subscriber != NULL, result);

    // subscribe

    const char *topics[] = { "test_topic_1" };
    QoS qoss[] = { QoS2 };
    status = smqtt_mt_subscribe(
            subscriber, 1, topics, qoss,
            subscribe_cb, sub_state);
    ASSERT_TRUE(status == SMQTT_MT_OK, result);

    bool got_callback_before_timeout = wait_for_cb(sub_state);
    ASSERT_TRUE(got_callback_before_timeout, result);
    ASSERT_TRUE(sub_state->subscription_response.topic_count == 1, result);
    ASSERT_TRUE(sub_state->subscription_response.success[0], result);

    // publish something

    status = smqtt_mt_publish(
            publisher, topics[0], "hello", QoS2, false, &request_cb, pub_state);
    ASSERT_TRUE(status == SMQTT_MT_OK, result);

    got_callback_before_timeout = wait_for_cb(pub_state);
    ASSERT_TRUE(got_callback_before_timeout, result);

    // check for receipt
    got_callback_before_timeout = wait_for_cb(sub_msg_state);
    ASSERT_TRUE(got_callback_before_timeout, result);
    ASSERT_TRUE(!strcmp(sub_msg_state->message_response.topic, topics[0]), result);
    ASSERT_TRUE(!strcmp(sub_msg_state->message_response.msg, "hello"), result);
    ASSERT_TRUE(sub_msg_state->message_response.qos == QoS2, result);
    ASSERT_TRUE(sub_msg_state->message_response.retain == false, result);
    release_cb(sub_msg_state);

    // disconnect

    status = smqtt_mt_disconnect(publisher);
    ASSERT_TRUE(status == SMQTT_MT_OK, result);

    status = smqtt_mt_disconnect(subscriber);
    ASSERT_TRUE(status == SMQTT_MT_OK, result);

    return result;
}

test_result_t
test_pub_sub_qos_mixed_v3()
{
    test_result_t result;
    init_result(&result);

    smqtt_mt_client_t *publisher = NULL;
    smqtt_mt_status_t status =
            smqtt_mt_connect_internal(REMOTE_SERVER, "127.0.0.1", 1883,
                                      "pub_client_1", "user1", "password1",30u, true,
                                      false, QoS0, false, NULL, NULL, NULL, NULL,
                                      &publisher);
    ASSERT_TRUE(status == SMQTT_MT_OK, result);
    ASSERT_TRUE(publisher != NULL, result);

    cb_state_t *pub_state_2 = create_cb_state();
    cb_state_t *pub_state_3 = create_cb_state();
    cb_state_t *sub_state = create_cb_state();
    cb_state_t *sub_msg_state = create_cb_state();

    smqtt_mt_client_t *subscriber = NULL;
    status =
            smqtt_mt_connect_internal(REMOTE_SERVER, "127.0.0.1", 1883,
                                      "sub_client_1", "user1", "password1",30u, true,
                                      false, QoS0, false, NULL, NULL,
                                      message_cb, sub_msg_state,
                                      &subscriber);

    ASSERT_TRUE(status == SMQTT_MT_OK, result);
    ASSERT_TRUE(subscriber != NULL, result);

    // subscribe

    const char *topics[] = { "test_topic_1" };
    QoS qoss[] = { QoS2 };
    status = smqtt_mt_subscribe(
            subscriber, 1, topics, qoss,
            subscribe_cb, sub_state);
    ASSERT_TRUE(status == SMQTT_MT_OK, result);

    bool got_callback_before_timeout = wait_for_cb(sub_state);
    ASSERT_TRUE(got_callback_before_timeout, result);
    ASSERT_TRUE(sub_state->subscription_response.topic_count == 1, result);
    ASSERT_TRUE(sub_state->subscription_response.success[0], result);

    // publish something

    status = smqtt_mt_publish(
            publisher, topics[0], "hello1", QoS0, false, NULL, NULL);
    ASSERT_TRUE(status == SMQTT_MT_OK, result);
    status = smqtt_mt_publish(
            publisher, topics[0], "hello2", QoS1, false, &request_cb, pub_state_2);
    ASSERT_TRUE(status == SMQTT_MT_OK, result);
    status = smqtt_mt_publish(
            publisher, topics[0], "hello3", QoS2, false, &request_cb, pub_state_3);
    ASSERT_TRUE(status == SMQTT_MT_OK, result);

    // callbacks not supported for QoS0 publish, so we didn't pass one in
    got_callback_before_timeout = wait_for_cb(pub_state_2);
    ASSERT_TRUE(got_callback_before_timeout, result);
    got_callback_before_timeout = wait_for_cb(pub_state_3);
    ASSERT_TRUE(got_callback_before_timeout, result);

    // check for receipt
    got_callback_before_timeout = wait_for_cb(sub_msg_state);
    ASSERT_TRUE(got_callback_before_timeout, result);
    ASSERT_TRUE(!strcmp(sub_msg_state->message_response.topic, topics[0]), result);
    ASSERT_TRUE(!strcmp(sub_msg_state->message_response.msg, "hello1"), result);
    ASSERT_TRUE(sub_msg_state->message_response.qos == QoS0, result);
    ASSERT_TRUE(sub_msg_state->message_response.retain == false, result);
    release_cb(sub_msg_state);

    got_callback_before_timeout = wait_for_cb(sub_msg_state);
    ASSERT_TRUE(got_callback_before_timeout, result);
    ASSERT_TRUE(!strcmp(sub_msg_state->message_response.topic, topics[0]), result);
    ASSERT_TRUE(!strcmp(sub_msg_state->message_response.msg, "hello2"), result);
    ASSERT_TRUE(sub_msg_state->message_response.qos == QoS1, result);
    ASSERT_TRUE(sub_msg_state->message_response.retain == false, result);
    release_cb(sub_msg_state);

    got_callback_before_timeout = wait_for_cb(sub_msg_state);
    ASSERT_TRUE(got_callback_before_timeout, result);
    ASSERT_TRUE(!strcmp(sub_msg_state->message_response.topic, topics[0]), result);
    ASSERT_TRUE(!strcmp(sub_msg_state->message_response.msg, "hello3"), result);
    ASSERT_TRUE(sub_msg_state->message_response.qos == QoS2, result);
    ASSERT_TRUE(sub_msg_state->message_response.retain == false, result);
    release_cb(sub_msg_state);

    // disconnect

    status = smqtt_mt_disconnect(publisher);
    ASSERT_TRUE(status == SMQTT_MT_OK, result);

    status = smqtt_mt_disconnect(subscriber);
    ASSERT_TRUE(status == SMQTT_MT_OK, result);

    return result;
}