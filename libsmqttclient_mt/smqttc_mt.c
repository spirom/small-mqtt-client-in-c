


#include <stdio.h>

#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#include <time.h>
#include <sys/select.h>

#include "smqttc_mt.h"
#include "smqttc_mt_internal.h"
#include "server.h"
#include "messages.h"
#include "connection.h"

#define BUFFER_SIZE ((size_t)1024)
static uint8_t send_buffer[BUFFER_SIZE];
static uint8_t receive_buffer[BUFFER_SIZE];



smqtt_mt_status_t
smqtt_mt_connect_internal(
        server_mode_t mode,
        const char* server_ip,
        uint32_t port,
        const char* client_id,
        const char *user,
        const char *pw,
        uint16_t keep_alive_sec,
        bool clean_session,
        bool last_will_and_testament,
        QoS will_qos,
        bool will_retain,
        const char *will_topic,
        const char *will_message,
        void (*msg_callback)(
                char *topic,
                char *msg,
                QoS qos,
                bool retain,
                void *context),
        void *msg_cb_context,
        smqtt_mt_client_t **client)
{
    server_t *server = NULL;
    smqtt_mt_status_t stat;
    smqtt_mt_client_t *connection =
            (smqtt_mt_client_t *) malloc(sizeof(smqtt_mt_client_t));
    if (connection == NULL) {
        return SMQTT_MT_NOMEM;
    } else {

        if (mode == REMOTE_SERVER) {
            server = create_network_server();
        } else {
            // local server
        }

        // check connection strings are within bounds
        if ((strlen(client_id) + 1 > sizeof(connection->clientid)) ||
            (strlen(server_ip) + 1 > sizeof(connection->hostname))) {
            fprintf(stderr, "failed to connect: client or server exceed limits\n");
            free(connection);
            return SMQTT_MT_OK;  // strings too large
        }

        connection->port = port;
        connection->publish_packet_id = 1u;
        connection->subscribe_packet_id = 1u;
        strcpy(connection->hostname, server_ip);
        strcpy(connection->clientid, client_id);

        connection->msg_callback = msg_callback;
        connection->msg_cb_context = msg_cb_context;

        // create the stuff we need to connect
        connection->connected = false;

        stat = status_from_net(server_connect(server,
                                              connection->hostname, connection->port, 100));
        if (stat != SMQTT_MT_OK) {
            fprintf(stderr, "failed to connect: to server socket\n");
            free(connection);
            return stat;
        }

        // send Connect message

        size_t send_len =
                make_connect_message(send_buffer, BUFFER_SIZE,
                                     connection->clientid, user, pw,
                                     keep_alive_sec, clean_session,
                                     last_will_and_testament, will_qos, will_retain,
                                     will_topic, will_message);

        stat = status_from_net(server_send(server, send_buffer, send_len));
        if (stat != SMQTT_MT_OK) {
            free(server);
            free(connection);
            return stat;
        }

        long sz = server_receive(server,
                                 receive_buffer, sizeof(receive_buffer));
        if (sz > 0) {
            response_t *resp = deserialize_response(receive_buffer, sz);
            if (resp == 0) {
                puts("failed to parse connection response");
                return 99;
            } else {
                if ((resp->type == CONNACK) &&
                    (resp->body.connack_data.type != Connection_Accepted)) {
                    smqtt_mt_status_t status = SMQTT_MT_NOACK;
                    switch (resp->body.connack_data.type) {
                        case Connection_Refused_unacceptable_protocol_version:
                            status = SMQTT_MT_PROTOCOL;
                            break;
                        case Connection_Refused_identifier_rejected:
                            status = SMQTT_MT_IDENTIFIER;
                            break;
                        case Connection_Refused_server_unavailable:
                            status = SMQTT_MT_SERVER;
                            break;
                        case Connection_Refused_bad_username_or_password:
                            status = SMQTT_MT_NOT_AUTHENTICATED;
                            break;
                        case Connection_Refused_not_authorized:
                            status = SMQTT_MT_NOT_AUTHORIZED;
                            break;
                        default:
                            break;
                    }
                    server_disconnect(server);
                    free(server);
                    free(connection);
                    free(resp);
                    return status;
                } else {
                    // connected
                    free(resp);
                }
            }
        } else {
            server_disconnect(server);
            free(server);
            free(connection);
            return SMQTT_MT_NOMESSAGE;
        }

        // set connected flag
        connection->connected = true;


        start_session_thread(connection, 10, 1024, server);


        *client = connection;
        return SMQTT_MT_OK;
    }
}

smqtt_mt_status_t
smqtt_mt_connect(
        const char *server_ip,
        uint32_t port,
        const char *client_id,
        const char *user,
        const char *pw,
        uint16_t keep_alive_sec,
        bool clean_session,
        bool last_will_and_testament,
        QoS will_qos,
        bool will_retain,
        const char *will_topic,
        const char *will_message,
        void (*msg_callback)(
                char *topic,
                char *msg,
                QoS qos,
                bool retain,
                void *context),
        void *msg_cb_context,
        smqtt_mt_client_t **client)
{
    return smqtt_mt_connect_internal(
            REMOTE_SERVER, server_ip, port, client_id,
            user, pw, keep_alive_sec, clean_session,
            last_will_and_testament, will_qos, will_retain,
            will_topic, will_message, msg_callback, msg_cb_context,
            client);
}

smqtt_mt_status_t
smqtt_mt_ping(smqtt_mt_client_t *client,
        uint16_t timeout_msec,
        void (*callback)(bool, void *),
        void *cb_context)
{
    if (!client->connected) {
        return SMQTT_MT_NOT_CONNECTED;
    } else {

        uint8_t *buffer;
        uint16_t slot;
        slot = get_buffer_slot(client, &buffer);
        size_t actual_len =
                make_pingreq_message(buffer, BUFFER_SIZE);

        waiting_t *waiting = (waiting_t *)malloc(sizeof(waiting_t));
        waiting->type = PINGRESP;
        waiting->callback = callback;
        waiting->cb_context = cb_context;
        enqueue_waiting(client, waiting);

        smqtt_mt_status_t stat =
                enqueue_slot_for_send(client, slot, actual_len);
        fprintf(stderr, "cli: message enqueued\n");


        if (stat != SMQTT_MT_OK) {
            return stat;
        }

    }

    return SMQTT_MT_OK;
}

smqtt_mt_status_t
smqtt_mt_publish(smqtt_mt_client_t *client,
              const char *topic,
              const char *msg,
              QoS qos,
              bool retain,
              void (*callback)(bool, void *),
              void *cb_context)
{
    if (!client->connected) {
        return SMQTT_MT_NOT_CONNECTED;
    } else if (qos > QoS2) {
        return SMQTT_MT_INVALID;
    } else {
        uint8_t *buffer;
        uint16_t slot;
        slot = get_buffer_slot(client, &buffer);
        size_t actual_len =
                make_publish_message(buffer, BUFFER_SIZE, topic,
                                     client->publish_packet_id, qos, retain, msg);

        switch (qos) {
            case QoS1: {
                waiting_t *waiting = (waiting_t *) malloc(sizeof(waiting_t));
                waiting->type = PUBACK;
                waiting->callback = callback;
                waiting->cb_context = cb_context;
                waiting->puback_data.packet_id = client->publish_packet_id;
                enqueue_waiting(client, waiting);
                client->publish_packet_id++; // TODO: MT safety
            }
            break;
            case QoS2: {
                waiting_t *waiting = (waiting_t *) malloc(sizeof(waiting_t));
                waiting->type = PUBREC;
                waiting->callback = callback;
                waiting->cb_context = cb_context;
                waiting->pubrec_data.packet_id = client->publish_packet_id;
                enqueue_waiting(client, waiting);
                client->publish_packet_id++; // TODO: MT safety
            }
                break;
            default:
                // no need to enqueue anything
                break;
        }

        smqtt_mt_status_t stat =
                enqueue_slot_for_send(client, slot, actual_len);

        if (stat != SMQTT_MT_OK) {
            return stat;
        }
    }
    return SMQTT_MT_OK;
}

smqtt_mt_status_t
smqtt_mt_subscribe(
        smqtt_mt_client_t *client,
        uint8_t topic_count,
        const char **topics,
        const QoS *qoss,
        void (*sub_callback)(
                bool completed,
                uint16_t packet_id,
                uint16_t topic_count,
                const bool success[],
                const QoS qoss[],
                void *context),
        void *sub_cb_context)
{
    if (!client->connected) {
        return SMQTT_MT_NOT_CONNECTED;
    } else {
        uint8_t *buffer;
        uint16_t slot;
        slot = get_buffer_slot(client, &buffer);
        size_t actual_len =
                make_subscribe_message(buffer, BUFFER_SIZE,
                                       client->subscribe_packet_id,
                                       topic_count, topics, qoss);
        client->subscribe_packet_id++;

        waiting_t *waiting = (waiting_t *) malloc(sizeof(waiting_t));
        waiting->type = SUBACK;
        waiting->callback = NULL;
        waiting->cb_context = sub_cb_context;
        waiting->suback_data.packet_id = client->publish_packet_id;
        waiting->suback_data.sub_callback = sub_callback;
        enqueue_waiting(client, waiting);
        client->publish_packet_id++; // TODO: MT safety

        smqtt_mt_status_t stat =
                enqueue_slot_for_send(client, slot, actual_len);

        if (stat != SMQTT_MT_OK) {
            return stat;
        }
    }
    return SMQTT_MT_OK;
}

smqtt_mt_status_t
smqtt_mt_disconnect(smqtt_mt_client_t *client)
{

    fprintf(stderr, "cli: disconnecting\n");
    signal_disconnect(client);

    smqtt_mt_status_t stat = join_session_thread(client);
    if (stat != SMQTT_MT_OK) {
        return stat;
    }

    size_t actual_len = make_disconnect_message(send_buffer, BUFFER_SIZE);
    stat =
            status_from_net(
                    server_send(client->server,
                            send_buffer, actual_len));

    // TODO: this should block for a while to be courteous
    server_disconnect(client->server);

    // TODO: maybe a force argument, but otherwise exit without
    // TODO: joining the thread if there was a problem
    if (stat != SMQTT_MT_OK) {
        return stat;
    }

    return SMQTT_MT_OK;
}

