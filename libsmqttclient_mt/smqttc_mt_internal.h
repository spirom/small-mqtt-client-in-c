#ifndef SMQTTC_SMQTTC_MT_INTERNAL_H
#define SMQTTC_SMQTTC_MT_INTERNAL_H

#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>

#include "smqttc_mt.h"
#include "server.h"
#include "connection.h"

#define MAX_XLIENT_ID_CHARS 24
#define MAX_HOSTNAME_CHARS 100

struct smqtt_mt_client
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


    session_state_t *session_state;
};

smqtt_mt_status_t
smqtt_mt_connect_internal(server_mode_t mode,
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
                       smqtt_mt_client_t **client);

#endif //SMQTTC_SMQTTC_MT_INTERNAL_H
