
#include <pthread.h>
#include <stdbool.h>
#include <malloc.h>
#include <messages.h>

#include "smqttc_mt.h"
#include "smqttc_mt_internal.h"
#include "server.h"
#include "connection.h"

#define BUFFER_SIZE ((size_t)1024)
// TODO: move to session
static uint8_t receive_buffer[BUFFER_SIZE];

static void *
session_thread_loop(void *);

void init_queue(queue_t *queue)
{
    queue->head = NULL;
    queue->tail = NULL;
}

bool
dequeue(queue_t *queue, void **payload)
{
    if (queue->head == NULL) {
        return false;
    }
    queue_entry_t *first = queue->head;
    if (queue->head != NULL) {
        if (queue->tail == queue->head) {
            queue->head = NULL;
            queue->tail = NULL;
        } else {
            queue->head = queue->head->next;
        }
    }
    *payload = first->payload;
    free(first);
    return true;
}

void
enqueue(queue_t *queue, void *payload)
{
    queue_entry_t *entry = malloc(sizeof(queue_entry_t));
    entry->payload = payload;
    entry->previous = NULL;
    entry->next = NULL;
    if (queue->head == NULL) {
        queue->head = entry;
        queue->tail = entry;
    } else {
        entry->previous = queue->tail;
        queue->tail = entry;
    }
}

session_state_t *
start_session_thread(
        slot_index_t buffer_count, size_t buffer_size,
        server_t *server) {


    session_state_t *session_state = malloc(sizeof(session_state_t));
    session_state->buffer_count = buffer_count;
    session_state->buffer_size = buffer_size;
    session_state->ready_to_disconnect = false;
    session_state->server = server;

    init_queue(&session_state->free_queue);
    init_queue(&session_state->send_queue);
    init_queue(&session_state->ack_queue);

    pthread_mutex_init(&session_state->thread_mutex, NULL);
    pthread_cond_init(&session_state->thread_loop_state_change, NULL);
    pthread_cond_init(&session_state->buffer_slot_state_change, NULL);

    session_state->buffer_slots =
            calloc(buffer_count, sizeof(buffer_slot_t));
    for (slot_index_t i = 0; i < buffer_count; i++) {
        buffer_slot_t *slot = &session_state->buffer_slots[i];
        //slot->callback = NULL;
        //slot->cb_context = NULL;
        slot->buffer = malloc(buffer_size);
        slot_index_rec_t *rec = malloc(sizeof(slot_index_rec_t));
        rec->index = i;
        enqueue(&session_state->free_queue, (void *)rec);
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

    // TODO: clear out queues

    free(session_state);
}

void *
session_thread_loop(void *arg)
{
    session_state_t *session_state = (session_state_t *)arg;
    for (;;) {
        int status = pthread_mutex_lock(&session_state->thread_mutex);
        bool has_message_to_send = false;
        slot_index_t position;
        bool ready = session_state->ready_to_disconnect;
        slot_index_rec_t *slot_rec;
        if (!ready) {
            has_message_to_send =
                    dequeue(&session_state->send_queue, (void **)&slot_rec);
        }
        status = pthread_mutex_unlock(&session_state->thread_mutex);
        if (ready) break;

        if (has_message_to_send) {
            position = slot_rec->index; // don't free as we use it below
            buffer_slot_t *slot = &session_state->buffer_slots[position];
            smqtt_mt_status_t stat =
                    server_send(session_state->server,
                                slot->buffer, slot->message_length);

            status = pthread_mutex_lock(&session_state->thread_mutex);
            enqueue(&session_state->free_queue, slot_rec);
            status = pthread_mutex_unlock(&session_state->thread_mutex);

            fprintf(stderr, "con: message sent\n");
        } else {
            fprintf(stderr, "con: no message to send\n");
        }


        long sz = server_receive(session_state->server,
                receive_buffer, sizeof(receive_buffer));
        if (sz > 0) {
            fprintf(stderr, "con: message received\n");

            response_t *resp = deserialize_response(receive_buffer, sz);
            switch (resp->type) {
                case PINGRESP: {
                    // TODO may not be the right one
                    waiting_t *waiting;
                    bool found =
                            dequeue(&session_state->ack_queue, (void **) &waiting);
                    if (found) {
                        waiting->callback(true, waiting->cb_context);
                    }
                }
                    break;
                default:
                    break;
            }

        } else {
            fprintf(stderr, "con: no message received\n");
        }

    }

    fprintf(stderr, "con: end of send/receive loop\n");

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
get_buffer_slot(session_state_t *session_state,
        uint8_t **buffer)
{
    int status = pthread_mutex_lock(&session_state->thread_mutex);
    uint16_t position;
    slot_index_rec_t *slot_rec;
    bool got_a_slot = dequeue(&session_state->free_queue, (void **)&slot_rec);;
    while (!got_a_slot) {
        // TODO: use cond var
        got_a_slot = dequeue(&session_state->free_queue, (void **)&slot_rec);
    }
    position = slot_rec->index;
    buffer_slot_t *slot = &session_state->buffer_slots[position];
    free(slot_rec);
    status = pthread_mutex_unlock(&session_state->thread_mutex);

    *buffer = slot->buffer;
    return position;
}

extern smqtt_mt_status_t
enqueue_slot_for_send(session_state_t *session_state,
                      slot_index_t position, uint16_t length)
{
    session_state->buffer_slots[position].message_length = length;
    slot_index_rec_t *rec = malloc(sizeof(slot_index_rec_t));
    rec->index = position;
    int status = pthread_mutex_lock(&session_state->thread_mutex);
    enqueue(&session_state->send_queue, rec);
    status = pthread_mutex_unlock(&session_state->thread_mutex);
    fprintf(stderr, "con: message enqueued\n");
    return SMQTT_MT_OK;
}

smqtt_status_t
enqueue_waiting(session_state_t *session_state,
        waiting_t *waiting)
{
    int status = pthread_mutex_lock(&session_state->thread_mutex);
    enqueue(&session_state->ack_queue, waiting);
    status = pthread_mutex_unlock(&session_state->thread_mutex);
    fprintf(stderr, "con: message enqueued\n");
    return SMQTT_MT_OK;
}





