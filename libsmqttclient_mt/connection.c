
#include <pthread.h>
#include <stdbool.h>
#include <malloc.h>
#include <messages.h>
#include <string.h>

#include "smqttc_mt.h"
#include "smqttc_mt_internal.h"
#include "server.h"
#include "connection.h"


smqtt_mt_status_t
status_from_net(smqtt_net_status_t stat)
{
    switch (stat) {
        case SMQTT_NET_OK:
            return SMQTT_MT_OK;
        default:
            return SMQTT_MT_SOCKET;
    }
}

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
remove_from_queue(queue_t *queue, queue_entry_t *entry)
{
    if (entry->previous == NULL) {
        queue->head = entry->next;
    } else {
        entry->previous->next = entry->next;
    }

    if (entry->next == NULL) {
        queue->tail = entry->previous;
    } else {
        entry->next->previous = entry->previous;
    }
}

bool
dequeue_by_type_and_id(
        queue_t *queue,
        message_type_t type,
        uint16_t packet_id,
        waiting_t **waiting) {
    if (queue->head != NULL) {
        queue_entry_t *curr_entry = queue->head;
        while (curr_entry != NULL) {
            waiting_t *curr_waiting = (waiting_t *) curr_entry->payload;
            if (curr_waiting->type == type) {
                switch (type) {
                    case PUBACK:
                        if (curr_waiting->puback_data.packet_id == packet_id) {
                            *waiting = curr_waiting;
                            remove_from_queue(queue, curr_entry);
                            return true;
                        }
                        break;
                    case PUBREC:
                        if (curr_waiting->pubrec_data.packet_id == packet_id) {
                            *waiting = curr_waiting;
                            remove_from_queue(queue, curr_entry);
                            return true;
                        }
                        break;
                    case PUBCOMP:
                        if (curr_waiting->pubcomp_data.packet_id == packet_id) {
                            *waiting = curr_waiting;
                            remove_from_queue(queue, curr_entry);
                            return true;
                        }
                        break;
                    case PUBREL:
                        if (curr_waiting->pubrel_data.packet_id == packet_id) {
                            *waiting = curr_waiting;
                            remove_from_queue(queue, curr_entry);
                            return true;
                        }
                        break;
                    case PINGRESP:
                        *waiting = curr_waiting;
                        remove_from_queue(queue, curr_entry);
                        return true;
                    case SUBACK:
                        if (curr_waiting->suback_data.packet_id == packet_id) {
                            *waiting = curr_waiting;
                            remove_from_queue(queue, curr_entry);
                            return true;
                        }
                        return true;
                    case UNSUBACK:
                        if (curr_waiting->unsuback_data.packet_id == packet_id) {
                            *waiting = curr_waiting;
                            remove_from_queue(queue, curr_entry);
                            return true;
                        }
                        break;
                    default:
                        break;
                }
            }
            curr_entry = curr_entry->next;
        }
    }
    return false;
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
        queue->tail->next = entry;
        queue->tail = entry;
    }
}

bool
start_session_thread(
        smqtt_mt_client_t *client,
        slot_index_t buffer_count, size_t buffer_size,
        server_t *server) {


    client->buffer_count = buffer_count;
    client->buffer_size = buffer_size;
    client->ready_to_disconnect = false;
    client->server = server;
    client->receive_buffer = malloc(buffer_size);

    init_queue(&client->free_queue);
    init_queue(&client->send_queue);
    init_queue(&client->ack_queue);

    pthread_mutex_init(&client->thread_mutex, NULL);
    pthread_cond_init(&client->thread_loop_state_change, NULL);
    pthread_cond_init(&client->buffer_slot_state_change, NULL);

    client->buffer_slots =
            calloc(buffer_count, sizeof(buffer_slot_t));
    for (slot_index_t i = 0; i < buffer_count; i++) {
        buffer_slot_t *slot = &client->buffer_slots[i];
        slot->buffer = malloc(buffer_size);
        slot_index_rec_t *rec = malloc(sizeof(slot_index_rec_t));
        rec->index = i;
        enqueue(&client->free_queue, (void *)rec);
    }

    pthread_attr_t attr;
    int status = pthread_attr_init(&attr);
    if (status != 0) {
        // TODO
    }

    status = pthread_create(&client->session_thread, &attr,
                            &session_thread_loop, client);
    if (status != 0) {
        // TODO
    }

    status = pthread_attr_destroy(&attr);
    if (status != 0) {
        // TODO
    }
    return true;
}

int
join_session_thread(smqtt_mt_client_t *client)
{

    int status = pthread_join(client->session_thread, NULL);
    if (status != 0) {
        // TODO
    }

    for (uint16_t i = 0; i < client->buffer_count; i++) {
        free(client->buffer_slots[i].buffer);
    }

    free(client->buffer_slots);

    // TODO: clear out queues

    free(client);
}

void maybe_deliver_message(smqtt_mt_client_t *client, response_t *response)
{
    if (client->msg_callback != NULL) {
        uint16_t topic_size =
                response->body.publish_data.topic_size;
        char *topic = malloc(topic_size + 1);
        strncpy(topic,
                response->body.publish_data.topic,
                response->body.publish_data.topic_size);
        topic[topic_size] = '\0';
        uint16_t message_size =
                response->body.publish_data.message_size;
        char *message = malloc(
                message_size + 1);
        strncpy(message,
                response->body.publish_data.message,
                response->body.publish_data.message_size);
        message[message_size] = '\0';
        client->msg_callback(
                topic,
                message,
                response->body.publish_data.qos,
                response->body.publish_data.retain,
                client->msg_cb_context);
    }
}

static char *
copy_publish_topic(response_t *response)
{
    uint16_t topic_size =
            response->body.publish_data.topic_size;
    char *topic = malloc(topic_size + 1);
    strncpy(topic,
            response->body.publish_data.topic,
            response->body.publish_data.topic_size);
    topic[topic_size] = '\0';
    return topic;
}

static char *
copy_publish_msg(response_t *response)
{
    uint16_t message_size =
            response->body.publish_data.message_size;
    char *message = malloc(
            message_size + 1);
    strncpy(message,
            response->body.publish_data.message,
            response->body.publish_data.message_size);
    message[message_size] = '\0';
    return message;
}


void *
session_thread_loop(void *arg)
{
    fprintf(stderr, "con: starting session loop\n");
    smqtt_mt_client_t *client = (smqtt_mt_client_t *)arg;
    for (;;) {
        int status = pthread_mutex_lock(&client->thread_mutex);
        bool has_message_to_send = false;
        slot_index_t position;
        bool ready = client->ready_to_disconnect;
        slot_index_rec_t *slot_rec;
        if (!ready) {
            has_message_to_send =
                    dequeue(&client->send_queue, (void **)&slot_rec);
        }
        status = pthread_mutex_unlock(&client->thread_mutex);
        if (ready) break;

        if (has_message_to_send) {
            position = slot_rec->index; // don't free as we use it below
            buffer_slot_t *slot = &client->buffer_slots[position];
            smqtt_mt_status_t stat =
                    status_from_net(
                            server_send(client->server,
                                slot->buffer, slot->message_length));

            status = pthread_mutex_lock(&client->thread_mutex);
            enqueue(&client->free_queue, slot_rec);
            status = pthread_mutex_unlock(&client->thread_mutex);

            fprintf(stderr, "con: message sent\n");
        } else {
            fprintf(stderr, "con: no message to send\n");
        }


        long sz = server_receive(client->server,
                                 client->receive_buffer,
                                 client->buffer_size);
        if (sz > 0) {
            fprintf(stderr, "con: message received\n");

            response_t *resp =
                    deserialize_response(client->receive_buffer, sz);
            switch (resp->type) {
                case PINGRESP: {
                    waiting_t *waiting;
                    status = pthread_mutex_lock(&client->thread_mutex);
                    bool found =
                            dequeue_by_type_and_id(
                                    &client->ack_queue,
                                    PINGRESP,
                                    0l,
                                    &waiting);
                    status = pthread_mutex_unlock(&client->thread_mutex);
                    if (found) {
                        waiting->callback(true, waiting->cb_context);
                        free(waiting);
                    }
                }
                    break;
                case PUBACK: {
                    // Qos1 case, callback but no response
                    waiting_t *waiting;
                    status = pthread_mutex_lock(&client->thread_mutex);
                    bool found = dequeue_by_type_and_id(
                            &client->ack_queue,
                            PUBACK,
                            resp->body.puback_data.packet_id,
                            &waiting);
                    status = pthread_mutex_unlock(&client->thread_mutex);
                    if (found) {
                        waiting->callback(true, waiting->cb_context);
                        free(waiting);
                    }
                }
                    break;
                case PUBREC: {
                    // QoS2 case part 1: response but no callback yet
                    waiting_t *waiting;
                    status = pthread_mutex_lock(&client->thread_mutex);
                    uint16_t packet_id = resp->body.pubrec_data.packet_id;
                    bool found = dequeue_by_type_and_id(
                            &client->ack_queue,
                            PUBREC,
                            packet_id,
                            &waiting);
                    status = pthread_mutex_unlock(&client->thread_mutex);
                    if (found) {
                        // no locking needed for this part as we are using the
                        // private buffer
                        size_t actual_len =
                                make_pubrel_message(
                                        client->receive_buffer,
                                        client->buffer_size,
                                        packet_id);
                        smqtt_mt_status_t stat =
                                status_from_net(
                                        server_send(
                                                client->server,
                                                client->receive_buffer,
                                                actual_len));
                        if (stat == SMQTT_MT_OK) {
                            // just reuse the existing one without abusing union
                            // semantics
                            waiting->type = PUBCOMP;
                            waiting->pubcomp_data.packet_id = packet_id;
                            // this locks under the hood so we're fine
                            enqueue_waiting(client, waiting);
                        }
                    }
                }
                    break;
                case PUBCOMP: {
                    // QoS2 case part 2: finally do the callback
                    waiting_t *waiting;
                    status = pthread_mutex_lock(&client->thread_mutex);
                    bool found = dequeue_by_type_and_id(
                            &client->ack_queue,
                            PUBCOMP,
                            resp->body.pubcomp_data.packet_id,
                            &waiting);
                    status = pthread_mutex_unlock(&client->thread_mutex);
                    if (found) {
                        waiting->callback(true, waiting->cb_context);
                        free(waiting);
                    }
                }
                    break;
                case SUBACK: {
                    waiting_t *waiting;
                    status = pthread_mutex_lock(&client->thread_mutex);
                    bool found = dequeue_by_type_and_id(
                            &client->ack_queue,
                            SUBACK,
                            resp->body.suback_data.packet_id,
                            &waiting);
                    status = pthread_mutex_unlock(&client->thread_mutex);
                    if (found && (waiting->suback_data.sub_callback != NULL)) {
                        waiting->suback_data.sub_callback(
                                true,
                                resp->body.suback_data.packet_id,
                                resp->body.suback_data.topic_count,
                                resp->body.suback_data.success,
                                resp->body.suback_data.qoss,
                                waiting->cb_context);
                        free(waiting);
                    }
                    break;
                }
                case UNSUBACK: {
                    waiting_t *waiting;
                    status = pthread_mutex_lock(&client->thread_mutex);
                    bool found = dequeue_by_type_and_id(
                            &client->ack_queue,
                            UNSUBACK,
                            resp->body.unsuback_data.packet_id,
                            &waiting);
                    status = pthread_mutex_unlock(&client->thread_mutex);
                    if (found && (waiting->callback != NULL)) {
                        waiting->callback(
                                true,
                                waiting->cb_context);
                        free(waiting);
                    }
                    break;
                }

                case PUBLISH: {
                    // only deliver if this is the QoS0 case
                    switch (resp->body.publish_data.qos) {
                        case QoS0:
                            maybe_deliver_message(client, resp);
                            break;
                        case QoS1: {
                            uint16_t packet_id = resp->body.publish_data.packet_id;
                            size_t actual_len =
                                    make_puback_message(
                                            client->receive_buffer,
                                            client->buffer_size,
                                            packet_id);
                            smqtt_mt_status_t stat =
                                    status_from_net(
                                            server_send(
                                                    client->server,
                                                    client->receive_buffer,
                                                    actual_len));
                            if (stat == SMQTT_MT_OK) {
                                // TODO: squirrel away for possible resend
                                maybe_deliver_message(client, resp);
                            }

                        }
                            break;
                        case QoS2:
                        {
                            uint16_t packet_id = resp->body.publish_data.packet_id;
                            size_t actual_len =
                                    make_pubrec_message(
                                            client->receive_buffer,
                                            client->buffer_size,
                                            packet_id);
                            smqtt_mt_status_t stat =
                                    status_from_net(
                                            server_send(
                                                    client->server,
                                                    client->receive_buffer,
                                                    actual_len));
                            if (stat == SMQTT_MT_OK) {
                                // TODO: squirrel away for possible resend
                                // TODO: enqueue for later PUBREL matching
                                waiting_t *waiting =
                                        (waiting_t *) malloc(sizeof(waiting_t));
                                waiting->type = PUBREL;
                                waiting->callback = NULL;
                                waiting->cb_context = NULL;
                                waiting->pubrel_data.topic =
                                        copy_publish_topic(resp);
                                waiting->pubrel_data.msg =
                                        copy_publish_msg(resp);
                                waiting->pubrel_data.retain =
                                    resp->body.publish_data.retain;
                                waiting->pubrel_data.packet_id = packet_id;
                                enqueue_waiting(client, waiting);
                                // don't deliver yet -- do that on PUBREL
                            }

                        }
                            break;
                        default:
                            break;
                    }
                }
                    break;
                case PUBREL: {
                    waiting_t *waiting;
                    status = pthread_mutex_lock(&client->thread_mutex);
                    uint16_t packet_id = resp->body.pubrel_data.packet_id;
                    bool found = dequeue_by_type_and_id(
                            &client->ack_queue,
                            PUBREL,
                            packet_id,
                            &waiting);
                    status = pthread_mutex_unlock(&client->thread_mutex);
                    if (found) {
                        // no locking needed for this part as we are using the
                        // private buffer
                        size_t actual_len =
                                make_pubcomp_message(
                                        client->receive_buffer,
                                        client->buffer_size,
                                        packet_id);
                        smqtt_mt_status_t stat =
                                status_from_net(
                                        server_send(
                                                client->server,
                                                client->receive_buffer,
                                                actual_len));
                        if (stat == SMQTT_MT_OK) {
                            if (client->msg_callback != NULL) {
                                client->msg_callback(
                                        waiting->pubrel_data.topic,
                                        waiting->pubrel_data.msg,
                                        QoS2, // or we wouldn't be here
                                        waiting->pubrel_data.retain,
                                        client->msg_cb_context);
                            }
                        }
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
signal_disconnect(smqtt_mt_client_t *client)
{
    int status = pthread_mutex_lock(&client->thread_mutex);
    client->ready_to_disconnect = true;
    status = pthread_mutex_unlock(&client->thread_mutex);
    return status;
}

extern uint16_t
get_buffer_slot(
        smqtt_mt_client_t *client,
        uint8_t **buffer)
{
    int status = pthread_mutex_lock(&client->thread_mutex);
    uint16_t position;
    slot_index_rec_t *slot_rec;
    bool got_a_slot = dequeue(&client->free_queue, (void **)&slot_rec);;
    while (!got_a_slot) {
        // TODO: use cond var
        got_a_slot = dequeue(&client->free_queue, (void **)&slot_rec);
    }
    position = slot_rec->index;
    buffer_slot_t *slot = &client->buffer_slots[position];
    free(slot_rec);
    status = pthread_mutex_unlock(&client->thread_mutex);

    *buffer = slot->buffer;
    return position;
}

extern smqtt_mt_status_t
enqueue_slot_for_send(smqtt_mt_client_t *client,
                      slot_index_t position, uint16_t length)
{
    client->buffer_slots[position].message_length = length;
    slot_index_rec_t *rec = malloc(sizeof(slot_index_rec_t));
    rec->index = position;
    int status = pthread_mutex_lock(&client->thread_mutex);
    enqueue(&client->send_queue, rec);
    status = pthread_mutex_unlock(&client->thread_mutex);
    fprintf(stderr, "con: message enqueued\n");
    return SMQTT_MT_OK;
}

smqtt_mt_status_t
enqueue_waiting(
        smqtt_mt_client_t *client,
        waiting_t *waiting)
{
    int status = pthread_mutex_lock(&client->thread_mutex);
    enqueue(&client->ack_queue, waiting);
    status = pthread_mutex_unlock(&client->thread_mutex);
    fprintf(stderr, "con: message callback enqueued\n");
    return SMQTT_MT_OK;
}





