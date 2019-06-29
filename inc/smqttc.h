
#ifndef SMQTTC_SMQTTC_H
#define SMQTTC_SMQTTC_H

#include <stdint.h>
#include <stdbool.h>

#include "protocol.h"

typedef struct smqtt_client smqtt_client_t;

typedef enum {
    SMQTT_OK = 0,
    SMQTT_BAD_MESSAGE,
    SMQTT_NOACK,
    SMQTT_NOMESSAGE,
    SMQTT_NOT_CONNECTED,
    SMQTT_UNEXPECTED,
    SMQTT_INVALID,
    SMQTT_NOT_AUTHORIZED,
    SMQTT_SOCKET,
    SMQTT_PROTOCOL,
    SMQTT_IDENTIFIER,
    SMQTT_SERVER,
    SMQTT_NOT_AUTHENTICATED
} smqtt_status_t;

smqtt_status_t
smqtt_ping(smqtt_client_t *client);

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
              smqtt_client_t **client);

smqtt_status_t
smqtt_disconnect(smqtt_client_t *client);

smqtt_status_t
smqtt_publish(smqtt_client_t *client,
              const char *topic,
              const char *msg,
              QoS qos,
              bool retain);

smqtt_status_t
smqtt_subscribe(smqtt_client_t *client,
                const char **topics,
                const QoS *qoss);

/**
 *
 * @param client
 * @param packet_id
 * @param message  Caller responsible for deleting if not null
 * @param message_size
 * @return
 */
smqtt_status_t
smqtt_get_next_message(smqtt_client_t *client,
                       uint16_t *packet_id,
                       char **message,
                       uint16_t *message_size);


smqtt_status_t
smqtt_unsubscribe(smqtt_client_t *client,
                  const char **topics);


#endif //SMQTTC_SMQTTC_H
