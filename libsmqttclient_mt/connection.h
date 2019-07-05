#ifndef SMQTTC_CONNECTION_H
#define SMQTTC_CONNECTION_H

#include <pthread.h>

#include "messages.h"

// TODO: abstract these structs away -- details should be private

typedef uint16_t slot_index_t;

typedef struct {
    message_type_t type;
    void (*callback)(bool, void *);
    void *cb_context;
} waiting_t;

typedef struct {
    slot_index_t index;
} slot_index_rec_t;

typedef struct {
    uint16_t message_length;
    uint8_t *buffer;
} buffer_slot_t;

typedef struct queue_entry {
    void *payload;
    struct queue_entry *previous;
    struct queue_entry *next;
} queue_entry_t;

typedef struct {
    queue_entry_t *head;
    queue_entry_t *tail;
} queue_t;

typedef struct {
    slot_index_t buffer_count;
    size_t buffer_size;
    buffer_slot_t *buffer_slots;
    pthread_t session_thread;
    pthread_mutex_t thread_mutex;
    pthread_cond_t thread_loop_state_change;
    pthread_cond_t buffer_slot_state_change;
    bool ready_to_disconnect;
    queue_t free_queue;
    queue_t send_queue;
    queue_t ack_queue;
    /**
     * Underlying server connection
     */
    server_t *          server;
} session_state_t;



static void *
session_thread_loop(void *);

/**
 * Also initializes buffer pool
 * @return
 */
extern session_state_t *
start_session_thread(slot_index_t buffer_count, size_t buffer_size, server_t *server);

/**
 * Also frees buffer pool
 * @param buffer fill in pointer to buffer to use
 * @return
 */
extern int
join_session_thread(session_state_t *);

extern uint16_t
get_buffer_slot(session_state_t *session_state,
        uint8_t **buffer);

/**
 * Makes slot available for async socket send
 * @param session_state
 * @param slot
 * @param length
 * @return
 */
extern smqtt_mt_status_t
enqueue_slot_for_send(session_state_t *session_state,
                      slot_index_t slot, uint16_t length);

extern smqtt_status_t
enqueue_waiting(session_state_t *session_state, waiting_t *waiting);

extern int
signal_disconnect(session_state_t *);

#endif //SMQTTC_CONNECTION_H
