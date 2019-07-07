#ifndef SMQTTC_SMQTTC_MT_INTERNAL_H
#define SMQTTC_SMQTTC_MT_INTERNAL_H

#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>

#include "smqttc_mt.h"
#include "server.h"
#include "connection.h"

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
        smqtt_mt_client_t **client);

#endif //SMQTTC_SMQTTC_MT_INTERNAL_H
