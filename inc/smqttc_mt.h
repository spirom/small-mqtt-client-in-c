

#ifndef SMQTTC_SMQTTC_MT_H
#define SMQTTC_SMQTTC_MT_H

#include <stdint.h>
#include <stdbool.h>

#include "protocol.h"

typedef enum {
    SMQTT_MT_OK = 0,
    SMQTT_MT_BAD_MESSAGE,
    SMQTT_MT_NOACK,
    SMQTT_MT_NOMESSAGE,
    SMQTT_MT_NOT_CONNECTED,
    SMQTT_MT_UNEXPECTED,
    SMQTT_MT_INVALID,
    SMQTT_MT_NOT_AUTHORIZED,
    SMQTT_MT_SOCKET,
    SMQTT_MT_PROTOCOL,
    SMQTT_MT_IDENTIFIER,
    SMQTT_MT_SERVER,
    SMQTT_MT_NOT_AUTHENTICATED
} smqtt_mt_status_t;

typedef struct smqtt_mt_client smqtt_mt_client_t;

smqtt_mt_status_t
smqtt_mt_connect(const char *server_ip,
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
              smqtt_mt_client_t **client);

smqtt_mt_status_t
smqtt_mt_ping(smqtt_mt_client_t *client, uint16_t timeout_msec);

smqtt_mt_status_t
smqtt_mt_disconnect(smqtt_mt_client_t *client);

#endif //SMQTTC_SMQTTC_MT_H
