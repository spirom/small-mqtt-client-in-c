

#ifndef SMQTTC_SMQTTC_INTERNAL_H
#define SMQTTC_SMQTTC_INTERNAL_H

#include "smqttc.h"
#include "server.h"

void
mqtt_drop(smqtt_client_t *broker);


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
        smqtt_client_t **client);

#endif //SMQTTC_SMQTTC_INTERNAL_H
