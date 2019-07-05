#include <sys/select.h>
#include <malloc.h>

#include "messages.h"
#include "messages_internal.h"
#include "smqttc_mt_internal.h"
#include "smqttc_mt.h"
#include "server.h"

#include "test_macros.h"

typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t cond;
    bool done;
} cb_state_t;

cb_state_t *create_cb_state(int efd)
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

void ping_cb(bool returned, void *context)
{
    printf("called back\n");
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

    cb_state_t *state1 = create_cb_state(50);
    cb_state_t *state2 = create_cb_state(50);

    status = smqtt_mt_ping(client1, 500, &ping_cb, state1);
    ASSERT_TRUE(status == SMQTT_MT_OK, result)

    status = smqtt_mt_ping(client1, 500, &ping_cb, state2);
    ASSERT_TRUE(status == SMQTT_MT_OK, result)

    bool got_callback_before_timeout = wait_for_cb(state1);
    ASSERT_TRUE(got_callback_before_timeout, result)

    got_callback_before_timeout = wait_for_cb(state2);
    ASSERT_TRUE(got_callback_before_timeout, result)

    status = smqtt_mt_disconnect(client1);
    ASSERT_TRUE(status == SMQTT_MT_OK, result)

    return result;
}