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
    union {
        struct {
            uint16_t        packet_id;
        } puback_data;
        struct {
            uint16_t        packet_id;
        } pubrec_data;
        struct {
            uint16_t        packet_id;
        } pubcomp_data;
        struct {
            char *          topic;
            char *          msg;
            bool            retain;
            uint16_t        packet_id;
        } pubrel_data;
        struct {
            void (*sub_callback)(
                    bool completed,
                    uint16_t packet_id,
                    uint16_t topic_count,
                    const bool success[],
                    const QoS qoss[],
                    void *context);
            uint16_t        packet_id;
        } suback_data;
        struct {
            uint16_t        packet_id;
        } unsuback_data;
    };
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

} session_state_t;

#define MAX_CLIENT_ID_CHARS 24
#define MAX_HOSTNAME_CHARS 100

struct smqtt_mt_client
{
    char                hostname[MAX_HOSTNAME_CHARS];
    char                clientid[MAX_CLIENT_ID_CHARS];
    uint16_t            port;
    /**
     * Is this currently connected?
     */
    bool                connected;
    /**
     * Keep track of the various packet ID sequences
     */
    uint16_t            publish_packet_id;
    uint16_t            subscribe_packet_id;
    uint16_t            unsubscribe_packet_id;


    slot_index_t buffer_count;
    size_t buffer_size;
    uint8_t *receive_buffer;
    buffer_slot_t *buffer_slots;
    pthread_t session_thread;
    pthread_mutex_t thread_mutex;
    pthread_cond_t thread_loop_state_change;
    pthread_cond_t buffer_slot_state_change;
    bool ready_to_disconnect;
    queue_t free_queue;
    queue_t send_queue;
    queue_t ack_queue;
    void (*msg_callback)(
            char *topic,
            char *msg,
            QoS qos,
            bool retain,
            void *context);
    void *msg_cb_context;
    /**
     * Underlying server connection
     */
    server_t *          server;
};

smqtt_mt_status_t
status_from_net(smqtt_net_status_t stat);

static void *
session_thread_loop(void *);

/**
 * Also initializes buffer pool
 * @return
 */
extern bool
start_session_thread(
        smqtt_mt_client_t *client,
        slot_index_t buffer_count,
        size_t buffer_size,
        server_t *server);

/**
 * Also frees buffer pool
 * @param buffer fill in pointer to buffer to use
 * @return
 */
extern int
join_session_thread(smqtt_mt_client_t *client);

extern uint16_t
get_buffer_slot(smqtt_mt_client_t *client,
        uint8_t **buffer);

/**
 * Makes slot available for async socket send
 * @param session_state
 * @param slot
 * @param length
 * @return
 */
extern smqtt_mt_status_t
enqueue_slot_for_send(smqtt_mt_client_t *client,
                      slot_index_t slot, uint16_t length);

extern smqtt_mt_status_t
enqueue_waiting(smqtt_mt_client_t *client, waiting_t *waiting);

extern int
signal_disconnect(smqtt_mt_client_t *client);

#endif //SMQTTC_CONNECTION_H
