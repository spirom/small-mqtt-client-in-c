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
    state->done = true;
    pthread_cond_signal(&state->cond);
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
    notify_cb_state((cb_state_t *) context);
}

bool wait_for_cb(cb_state_t *state)
{
    pthread_mutex_lock(&state->lock);
    while (!state->done)
        pthread_cond_wait(&state->cond, &state->lock);
    pthread_mutex_unlock(&state->lock);
    return true;
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
                                   false, QoS0, false, NULL, NULL,
                                   &client1);

    ASSERT_TRUE(status == SMQTT_MT_OK, result)
    ASSERT_TRUE(client1 != NULL, result)

    status = smqtt_mt_disconnect(client1);
    ASSERT_TRUE(status == SMQTT_MT_OK, result)

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
                                      false, QoS0, false, NULL, NULL,
                                      &client1);

    ASSERT_TRUE(status == SMQTT_MT_OK, result)
    ASSERT_TRUE(client1 != NULL, result)

    cb_state_t *state1 = create_cb_state();
    cb_state_t *state2 = create_cb_state();

    status = smqtt_mt_ping(client1, 500, &request_cb, state1);
    ASSERT_TRUE(status == SMQTT_MT_OK, result)

    status = smqtt_mt_ping(client1, 500, &request_cb, state2);
    ASSERT_TRUE(status == SMQTT_MT_OK, result)

    bool got_callback_before_timeout = wait_for_cb(state1);
    ASSERT_TRUE(got_callback_before_timeout, result)

    got_callback_before_timeout = wait_for_cb(state2);
    ASSERT_TRUE(got_callback_before_timeout, result)

    status = smqtt_mt_disconnect(client1);
    ASSERT_TRUE(status == SMQTT_MT_OK, result)

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
                                      false, QoS0, false, NULL, NULL,
                                      &client1);

    ASSERT_TRUE(status == SMQTT_MT_OK, result)
    ASSERT_TRUE(client1 != NULL, result)

    status = smqtt_mt_publish(
            client1, "test_topic", "hello", QoS0, false, NULL, NULL);
    ASSERT_TRUE(status == SMQTT_MT_OK, result)

    test_sleep(100); // give it time to send

    status = smqtt_mt_disconnect(client1);
    ASSERT_TRUE(status == SMQTT_MT_OK, result)

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
                                      false, QoS0, false, NULL, NULL,
                                      &client1);

    cb_state_t *state1 = create_cb_state();
    cb_state_t *state2 = create_cb_state();

    ASSERT_TRUE(status == SMQTT_MT_OK, result)
    ASSERT_TRUE(client1 != NULL, result)

    status = smqtt_mt_publish(
            client1, "test_topic", "hello", QoS1, false, &request_cb, state1);
    ASSERT_TRUE(status == SMQTT_MT_OK, result)

    status = smqtt_mt_publish(
            client1, "test_topic", "hello", QoS1, false, &request_cb, state2);
    ASSERT_TRUE(status == SMQTT_MT_OK, result)

    bool got_callback_before_timeout = wait_for_cb(state1);
    ASSERT_TRUE(got_callback_before_timeout, result)

    got_callback_before_timeout = wait_for_cb(state2);
    ASSERT_TRUE(got_callback_before_timeout, result)

    status = smqtt_mt_disconnect(client1);
    ASSERT_TRUE(status == SMQTT_MT_OK, result)

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
                                      false, QoS0, false, NULL, NULL,
                                      &client1);

    cb_state_t *state1 = create_cb_state();
    cb_state_t *state2 = create_cb_state();

    ASSERT_TRUE(status == SMQTT_MT_OK, result)
    ASSERT_TRUE(client1 != NULL, result)

    status = smqtt_mt_publish(
            client1, "test_topic", "hello", QoS2, false, &request_cb, state1);
    ASSERT_TRUE(status == SMQTT_MT_OK, result)

    status = smqtt_mt_publish(
            client1, "test_topic", "hello", QoS2, false, &request_cb, state2);
    ASSERT_TRUE(status == SMQTT_MT_OK, result)

    bool got_callback_before_timeout = wait_for_cb(state1);
    ASSERT_TRUE(got_callback_before_timeout, result)

    got_callback_before_timeout = wait_for_cb(state2);
    ASSERT_TRUE(got_callback_before_timeout, result)

    status = smqtt_mt_disconnect(client1);
    ASSERT_TRUE(status == SMQTT_MT_OK, result)

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
                                      false, QoS0, false, NULL, NULL,
                                      &client1);

    cb_state_t *state1 = create_cb_state();

    ASSERT_TRUE(status == SMQTT_MT_OK, result)
    ASSERT_TRUE(client1 != NULL, result)

    const char *topics[] = { "test_topic" };
    QoS qoss[] = { QoS1 };
    status = smqtt_mt_subscribe(
            client1, 1, topics, qoss, subscribe_cb, state1, NULL);
    ASSERT_TRUE(status == SMQTT_MT_OK, result)

    bool got_callback_before_timeout = wait_for_cb(state1);
    ASSERT_TRUE(got_callback_before_timeout, result)

    status = smqtt_mt_disconnect(client1);
    ASSERT_TRUE(status == SMQTT_MT_OK, result)

    return result;
}