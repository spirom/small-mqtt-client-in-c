
#include <stdio.h>

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>


#include "smqttc.h"
#include "server.h"
#include "messages.h"

static void debug_print(char *buffer, size_t len) {
    for (int i = 0; i < len; i++) {
        putchar(buffer[i]);
    }
}

#define MAX_XLIENT_ID_CHARS 24
#define MAX_HOSTNAME_CHARS 100

struct smqtt_client
{
	char                hostname[MAX_HOSTNAME_CHARS];
	char                clientid[MAX_XLIENT_ID_CHARS];
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
    /**
     * Underlying server connection
     */
    server_t *          server;
};

#define BUFFER_SIZE ((size_t)1024)
uint8_t send_buffer[BUFFER_SIZE];
uint8_t receive_buffer[BUFFER_SIZE];

smqtt_status_t
smqtt_connect_internal(server_mode_t mode,
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
             smqtt_client_t **client)
 {

    smqtt_client_t *connection = (smqtt_client_t *) malloc(sizeof(smqtt_client_t));
    if (connection != 0) {

        if (mode == REMOTE_SERVER) {
            connection->server = create_network_server();
        } else {
            // local server
        }

        // check connection strings are within bounds
        if ((strlen(client_id) + 1 > sizeof(connection->clientid)) || (strlen(server_ip) + 1 > sizeof(connection->hostname))) {
            fprintf(stderr, "failed to connect: client or server exceed limits\n");
            free(connection);
            return SMQTT_OK;  // strings too large
        }

        connection->port = port;
        connection->publish_packet_id = 0u;
        connection->subscribe_packet_id = 1u;
        strcpy(connection->hostname, server_ip);
        strcpy(connection->clientid, client_id);

        // create the stuff we need to connect
        connection->connected = false;

        smqtt_status_t stat =
                server_connect(connection->server,
                        connection->hostname, connection->port, 30000);
        if (stat != SMQTT_OK) {
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

        stat = server_send(connection->server, send_buffer, send_len);
        if (stat != SMQTT_OK) {
            free(connection->server);
            free(connection);
            return stat;
        }

        long sz = server_receive(connection->server,
                receive_buffer, sizeof(receive_buffer));
        if (sz > 0) {
            response_t *resp = deserialize_response(receive_buffer, sz);
            if (resp == 0) {
                puts("failed to parse connection response");
                return 99;
            } else {
                if ((resp->type == CONNACK) &&
                    (resp->body.connack_data.type != Connection_Accepted)) {
                    smqtt_status_t status = SMQTT_NOACK;
                    switch (resp->body.connack_data.type) {
                        case Connection_Refused_unacceptable_protocol_version:
                            status = SMQTT_PROTOCOL;
                            break;
                        case Connection_Refused_identifier_rejected:
                            status = SMQTT_IDENTIFIER;
                            break;
                        case Connection_Refused_server_unavailable:
                            status = SMQTT_SERVER;
                            break;
                        case Connection_Refused_bad_username_or_password:
                            status = SMQTT_NOT_AUTHENTICATED;
                            break;
                        case Connection_Refused_not_authorized:
                            status = SMQTT_NOT_AUTHORIZED;
                            break;
                        default:
                            break;
                    }
                    server_disconnect(connection->server);
                    free(connection->server);
                    free(connection);
                    free(resp);
                    return status;
                } else {
                    // connected
                    free(resp);
                }
            }
        } else {
            server_disconnect(connection->server);
            free(connection->server);
            free(connection);
            return SMQTT_NOMESSAGE;
        }

        // set connected flag
        connection->connected = true;

    }
	
	*client = connection;
    return SMQTT_OK;
}

smqtt_status_t
smqtt_connect(const char *server_ip,
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
              smqtt_client_t **client)
{
    return smqtt_connect_internal(REMOTE_SERVER, server_ip, port, client_id,
                user, pw, keep_alive_sec, clean_session,
                last_will_and_testament, will_qos, will_retain,
                will_topic, will_message, client);
}

smqtt_status_t
smqtt_ping(smqtt_client_t *client)
{
    if (!client->connected) {
        return SMQTT_NOT_CONNECTED;
    } else {

        size_t actual_len =
                make_pingreq_message(send_buffer, BUFFER_SIZE);

        smqtt_status_t stat =
                server_send(client->server, send_buffer, actual_len);
        if (stat != SMQTT_OK) {
            return stat;
        }

        long sz = server_receive(client->server,
                                 receive_buffer, sizeof(receive_buffer));  // wait for PINGACK

        response_t *resp = deserialize_response(receive_buffer, sz);
        if (resp == 0) {
            puts("failed to parse ping response");
            return SMQTT_BAD_MESSAGE;
        } else if (resp->type == PINGRESP) {
            free(resp);
        }
        else
        {
            puts("failed to ping");
            free(resp);
            return SMQTT_UNEXPECTED;
        }
    }

    return SMQTT_OK;
}

smqtt_status_t
smqtt_subscribe(smqtt_client_t *client,
        const char **topics, const QoS *qoss)
{
	if (!client->connected) {
        return SMQTT_NOT_CONNECTED;
	}

	client->subscribe_packet_id++;

    size_t actual_len =
            make_subscribe_message(send_buffer, BUFFER_SIZE,
                                 client->subscribe_packet_id, topics, qoss);

    smqtt_status_t stat = server_send(client->server, send_buffer, actual_len);
    if (stat != SMQTT_OK) {
        puts("failed to send subscribe message");
		return -1;
    }

    long sz = server_receive(client->server,
            receive_buffer, sizeof(receive_buffer));  // wait for SUBACK

    response_t *resp = deserialize_response(receive_buffer, sz);

    if (resp == 0) {
        puts("failed to parse subscription response");
        return SMQTT_BAD_MESSAGE;
    } else {
        if ((resp->type == SUBACK) &&
            (resp->body.suback_data.packet_id == client->subscribe_packet_id)) {

            free(resp);
        } else {
            puts("failed to subscribe");
            free(resp);
            return SMQTT_UNEXPECTED;
        }
    }

	return SMQTT_OK;
}

smqtt_status_t
smqtt_unsubscribe(smqtt_client_t *client, const char **topics)
{
    if (!client->connected) {
        return SMQTT_NOT_CONNECTED;
    }

    client->unsubscribe_packet_id++;

    size_t actual_len =
            make_unsubscribe_message(send_buffer, BUFFER_SIZE,
                                   client->unsubscribe_packet_id, topics);

    smqtt_status_t stat = server_send(client->server, send_buffer, actual_len);
    if (stat != SMQTT_OK) {
        puts("failed to send subscribe message");
        return stat;
    }

    long sz = server_receive(client->server,
                             receive_buffer, sizeof(receive_buffer));  // wait for UNSUBACK

    response_t *resp = deserialize_response(receive_buffer, sz);

    if (resp == 0) {
        puts("failed to parse unsubscription response");
        return SMQTT_BAD_MESSAGE;
    } else {
        if ((resp->type == UNSUBACK) &&
            (resp->body.suback_data.packet_id == client->unsubscribe_packet_id)) {

            free(resp);
        } else {
            puts("failed to unsubscribe");
            free(resp);
            return SMQTT_UNEXPECTED;
        }
    }

    return SMQTT_OK;
}

smqtt_status_t
smqtt_get_next_message(smqtt_client_t *client,
                       uint16_t *packet_id, char **message, uint16_t *message_size)
{
    long sz = server_receive(client->server,
                             receive_buffer, sizeof(receive_buffer));
    if (sz < 0) {
        return SMQTT_NOMESSAGE;
    } else {
        response_t *resp = deserialize_response(receive_buffer, sz);
        if (resp == 0) {
            return SMQTT_BAD_MESSAGE;
        } else if (resp->type == PUBLISH) {
            if (resp->body.publish_data.packet_id != 0u) {
                *packet_id = resp->body.publish_data.packet_id;
            }
            *message_size = resp->body.publish_data.message_size;
            *message = malloc(*message_size);
            strncpy(*message, resp->body.publish_data.message, resp->body.publish_data.message_size);

            if (resp->body.publish_data.qos == QoS0) {
                // done
                free(resp);
                return SMQTT_OK;
            } else if (resp->body.publish_data.qos == QoS1) {
                size_t actual_len =
                        make_puback_message(send_buffer, BUFFER_SIZE,
                                            resp->body.publish_data.packet_id);
                smqtt_status_t stat = server_send(client->server,
                        send_buffer, actual_len);
                if (stat != SMQTT_OK) {
                    puts("failed to PUBACK");
                    free(resp);
                    return stat;
                } else {
                    return SMQTT_OK;
                }
            } else if (resp->body.publish_data.qos == QoS2) {
                uint16_t packet_id = resp->body.publish_data.packet_id;
                size_t actual_len =
                        make_pubrec_message(send_buffer, BUFFER_SIZE, packet_id);

                smqtt_status_t stat = server_send(client->server,
                        send_buffer, actual_len);
                if (stat != SMQTT_OK) {
                    puts("failed to PUBREC");
                    free(resp);
                    return stat;
                } else {
                    free(resp);

                    sz = server_receive(client->server,
                                        receive_buffer, sizeof(receive_buffer));

                    resp = deserialize_response(receive_buffer, sz);
                    if (resp == 0) {
                        return SMQTT_BAD_MESSAGE;
                    } else if (resp->type == PUBREL &&
                               (resp->body.pubrel_data.packet_id == packet_id)) {
                        actual_len =
                                make_pubcomp_message(send_buffer, BUFFER_SIZE,
                                                    resp->body.pubrel_data.packet_id);
                        smqtt_status_t stat = server_send(client->server,
                                send_buffer, actual_len);
                        if (stat != SMQTT_OK) {
                            puts("failed to PUBCOMP");
                            free(resp);
                            return stat;
                        } else {
                            free(resp);
                            return SMQTT_OK;
                        }

                    } else {
                        free(resp);
                        return SMQTT_UNEXPECTED;
                    }

                }
            } else {
                free(resp);
                return SMQTT_OK;
            }
        } else {
            free(resp);
            return SMQTT_UNEXPECTED;
        }
    }

}

smqtt_status_t
smqtt_publish(smqtt_client_t *client,
                  const char *topic,
                  const char *msg,
                  QoS qos,
                  bool retain)
{
	if (!client->connected) {
        return SMQTT_NOT_CONNECTED;
	}
    if (qos > QoS2) {
        return SMQTT_INVALID;
    }
	
    if (qos == QoS0) {  


        size_t actual_len =
                make_publish_message(send_buffer, BUFFER_SIZE, topic,
                        client->publish_packet_id, qos, retain, msg);

        smqtt_status_t stat =
                server_send(client->server, send_buffer, actual_len);
        if (stat != SMQTT_OK) {
            return stat;
        }
    }
    else {
        client->publish_packet_id++;

        size_t actual_len =
                make_publish_message(send_buffer, BUFFER_SIZE, topic,
                                     client->publish_packet_id, qos, retain, msg);

        smqtt_status_t stat =
                server_send(client->server, send_buffer, actual_len);
        if (stat != SMQTT_OK) {
            return stat;
        }
        if (qos == QoS1) {
            // expect an acknowledgement with a message ID
            long sz = server_receive(client->server,
                    receive_buffer, sizeof(receive_buffer));  // wait for PUBACK

            response_t *resp = deserialize_response(receive_buffer, sz);
            if (resp == 0) {
                puts("failed to parse publish response");
                return SMQTT_BAD_MESSAGE;
            } else if (
                    (resp->type == PUBACK) &&
                    (resp->body.puback_data.packet_id == client->publish_packet_id)) {
                free(resp);
            }
            else
            {
                puts("failed to publish at QoS1");
                free(resp);
                return SMQTT_UNEXPECTED;
            }
        } else { // qos == QoS2
            // expect an acknowledgement with a message ID
            long sz = server_receive(client->server,
                                     receive_buffer, sizeof(receive_buffer));  // wait for PUBREC

            response_t *resp = deserialize_response(receive_buffer, sz);
            if (resp == 0) {
                puts("failed to parse publish response");
                return -1;
            } else if (
                    (resp->type == PUBREC) &&
                    (resp->body.puback_data.packet_id == client->publish_packet_id)) {
                free(resp);

                actual_len = make_pubrel_message(send_buffer, BUFFER_SIZE, client->publish_packet_id);
                smqtt_status_t stat =
                        server_send(client->server, send_buffer, actual_len);
                if (stat != SMQTT_OK) {
                    return stat;
                }

                long sz = server_receive(client->server,
                                         receive_buffer, sizeof(receive_buffer));  // wait for PUBCOMP

                response_t *resp = deserialize_response(receive_buffer, sz);
                if (resp == 0) {
                    puts("failed to parse publish response");
                    return SMQTT_BAD_MESSAGE;
                } else if (
                        (resp->type == PUBCOMP) &&
                        (resp->body.puback_data.packet_id == client->publish_packet_id)) {
                    free(resp);
                }
                else
                {
                    puts("failed to complete publish at QoS2");
                    free(resp);
                    return SMQTT_UNEXPECTED;
                }
            }
            else
            {
                puts("failed to publish at QoS2");
                free(resp);
                return SMQTT_UNEXPECTED;
            }

        }
        
    }
    
	return SMQTT_OK;
}


smqtt_status_t
smqtt_disconnect(smqtt_client_t *client)
{
    size_t actual_len = make_disconnect_message(send_buffer, BUFFER_SIZE);
    smqtt_status_t stat =
            server_send(client->server, send_buffer, actual_len);

    server_disconnect(client->server);
    if (stat != SMQTT_OK) {
        puts("failed to disconnect");
        return stat;
    }
    return SMQTT_OK;
}

/**
 * For testing only: drop the connection without send a disconnect message
 * @param broker
 */
smqtt_status_t
mqtt_drop(smqtt_client_t *broker)
{
    server_disconnect(broker->server);
    return SMQTT_OK;
}
