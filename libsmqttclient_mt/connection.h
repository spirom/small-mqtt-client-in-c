#ifndef SMQTTC_CONNECTION_H
#define SMQTTC_CONNECTION_H

#include <pthread.h>

// TODO: abstract these structs away -- details should be private

typedef struct {
    size_t used;
    uint8_t *buffer;
} buffer_slot_t;

typedef struct {
    uint16_t buffer_count;
    size_t buffer_size;
    buffer_slot_t *buffer_slots;
    pthread_t session_thread;
    pthread_mutex_t thread_mutex;
    pthread_cond_t thread_loop_state_change;
    bool ready_to_disconnect;
} session_state_t;

static void *
session_thread_loop(void *);

/**
 * Also initializes buffer pool
 * @return
 */
extern session_state_t *
start_session_thread(uint16_t buffer_count, size_t buffer_size);

/**
 * Also frees buffer pool
 * @param buffer fill in pointer to buffer to use
 * @return
 */
extern int
join_session_thread(session_state_t *);

extern uint16_t
get_buffer_slot(session_state_t *session_state, uint8_t **buffer);

/**
 * Also frees this slot
 * @param slot
 * @return result
 */
extern smqtt_mt_status_t
send_buffer_from_slot(session_state_t *session_state,
        uint16_t slot, uint16_t length);

extern int
signal_disconnect(session_state_t *);

#endif //SMQTTC_CONNECTION_H
