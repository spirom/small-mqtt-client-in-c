
#include <pthread.h>
#include <stdbool.h>
#include <malloc.h>

#include "smqttc_mt.h"
#include "smqttc_mt_internal.h"
#include "server.h"
#include "connection.h"

void *
session_thread_loop(void *);

session_state_t *
start_session_thread(uint16_t buffer_count, size_t buffer_size) {


    session_state_t *session_state = malloc(sizeof(session_state_t));
    session_state->buffer_count = buffer_count;
    session_state->buffer_size = buffer_size;
    session_state->ready_to_disconnect = false;

    pthread_mutex_init(&session_state->thread_mutex, NULL);
    pthread_cond_init(&session_state->thread_loop_state_change, NULL);

    session_state->buffer_slots =
            calloc(buffer_count, sizeof(buffer_slot_t));
    for (uint16_t i = 0; i < buffer_count; i++) {
        session_state->buffer_slots[i].buffer = malloc(buffer_size);
    }

    pthread_attr_t attr;
    int status = pthread_attr_init(&attr);
    if (status != 0) {
        // TODO
    }

    status = pthread_create(&session_state->session_thread, &attr,
                            &session_thread_loop, session_state);
    if (status != 0) {
        // TODO
    }

    status = pthread_attr_destroy(&attr);
    if (status != 0) {
        // TODO
    }
    return session_state;
}

int
join_session_thread(session_state_t *session_state)
{

    int status = pthread_join(session_state->session_thread, NULL);
    if (status != 0) {
        // TODO
    }

    for (uint16_t i = 0; i < session_state->buffer_count; i++) {
        free(session_state->buffer_slots[i].buffer);
    }

    free(session_state->buffer_slots);

    free(session_state);
}

void *
session_thread_loop(void *arg)
{
    session_state_t *session_state = (session_state_t *)arg;
    for (;;) {
        int status = pthread_mutex_lock(&session_state->thread_mutex);
        bool ready = session_state->ready_to_disconnect;
        status = pthread_mutex_unlock(&session_state->thread_mutex);
        if (ready) break;
        // TODO: pick a buffer and use it
        /*
        long sz = server_receive(connection->server,
        receive_buffer, sizeof(receive_buffer));

        if (sz > 0) {

        } else {
        break;
        }
        */
    }

    return NULL;
}

int
signal_disconnect(session_state_t *session_state)
{
    int status = pthread_mutex_lock(&session_state->thread_mutex);
    session_state->ready_to_disconnect = true;
    status = pthread_mutex_unlock(&session_state->thread_mutex);
    return status;
}

extern uint16_t
get_buffer_slot(session_state_t *session_state, uint8_t **buffer)
{
    // TODO: propper locked allocation
    *buffer = session_state->buffer_slots[0].buffer;
    return 0;
}

extern smqtt_mt_status_t
send_buffer_from_slot(session_state_t *session_state,
        uint16_t slot, uint16_t length)
{
    // TODO: lock

    // TODO: put in priority order

    // TODO: kick the loop (but don't wait for it)

    return SMQTT_MT_OK;
}

// TODO: figure out what to do when the response comes in, and who to notify


