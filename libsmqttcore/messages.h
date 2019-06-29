
#ifndef SMQTTC_MESSAGES_H
#define SMQTTC_MESSAGES_H

#include "protocol.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * It's easier to conflate the request and response type because
 * that's how the MQTT protocol organizes them.
 */
typedef enum {
    CONNECT = 1,
    CONNACK,
    PUBLISH,
    PUBACK,
    PUBREC,
    PUBREL,
    PUBCOMP,
    SUBSCRIBE,
    SUBACK,
    UNSUBSCRIBE,
    UNSUBACK,
    PINGREQ,
    PINGRESP,
    DISCONNECT
} message_type_t;

typedef enum {
    Connection_Accepted,
    Connection_Refused_unacceptable_protocol_version,
    Connection_Refused_identifier_rejected,
    Connection_Refused_server_unavailable,
    Connection_Refused_bad_username_or_password,
    Connection_Refused_not_authorized
} connack_msg_t;

#define MAX_TOPICS 32

// TODO: maybe these are just messages after all
typedef struct response_t {
    message_type_t type;
    union {
        struct {
            uint16_t        name_size;
            char *          name;
            uint8_t         version;
            bool            has_user;
            bool            has_pw;
            bool            has_will;
            QoS             will_qos;
            bool            will_retain;
            bool            has_clean;
            uint16_t        keep_alive;
            uint16_t        client_id_size;
            char *          client_id;
            uint16_t        will_topic_size;
            char *          will_topic;
            uint16_t        will_message_size;
            char *          will_message;
            uint16_t        user_size;
            char *          user;
            uint16_t        pw_size;
            char *          pw;
        } connect_data;
        struct {
            connack_msg_t   type;
        } connack_data;
        struct {
            QoS             qos;
            bool            retain;
            uint16_t        topic_size;
            char *          topic;
            uint16_t        packet_id;
            uint16_t        message_size;
            char *          message;
        } publish_data;
        struct {
            uint16_t        packet_id;
        } puback_data;
        struct {
            uint16_t        packet_id;
        } pubrec_data;
        struct {
            uint16_t        packet_id;
        } pubrel_data;
        struct {
            uint16_t        packet_id;
        } pubcomp_data;
        struct {
            uint16_t        packet_id;
            uint16_t        topic_lengths[MAX_TOPICS];
            char *          topics[MAX_TOPICS];
            QoS             qoss[MAX_TOPICS];
            uint16_t        topic_count;
        } subscribe_data;
        struct {
            uint16_t        packet_id;
            QoS             qos;
        } suback_data;
        struct {
            uint16_t        topic_lengths[MAX_TOPICS];
            char *          topics[MAX_TOPICS];
            uint16_t        topic_count;
            uint16_t        packet_id;
        } unsubscribe_data;
        struct {
            uint16_t        packet_id;
        } unsuback_data;
        // no data for PINGREQ
        // no data for PINGRESP
        // no data for disconnect
    } body;
} response_t;

response_t *
deserialize_response(uint8_t *buffer, size_t length);

size_t
make_connect_message(
        uint8_t *buffer, size_t length,
        const char *clientid,
        const char *user,
        const char *pw,
        uint8_t keep_alive_sec,
        bool clean_session,
        bool last_will_and_testament,
        QoS will_qos,
        bool will_retain,
        const char *will_topic,
        const char *will_message);

size_t
make_connack_message(
        uint8_t *buffer, size_t length,
        connack_msg_t type);

size_t
make_publish_message(
        uint8_t *buffer, size_t length,
        const char *topic,
        uint16_t packet_id,
        QoS qos,
        bool retain,
        const char *msg);

size_t
make_puback_message(
        uint8_t *buffer, size_t length,
        uint16_t packet_id);

size_t
make_pubrec_message(
        uint8_t *buffer, size_t length,
        uint16_t packet_id);

size_t
make_pubrel_message(
        uint8_t *buffer, size_t length,
        uint16_t packet_id);

size_t
make_pubcomp_message(
        uint8_t *buffer, size_t length,
        uint16_t packet_id);

size_t
make_subscribe_message(
        uint8_t *buffer, size_t length,
        uint16_t packet_id,
        const char *topics[],
        const QoS qoss[]);

size_t
make_unsubscribe_message(
        uint8_t *buffer, size_t length,
        uint16_t packet_id,
        const char *topic[]);

size_t
make_suback_message(
        uint8_t *buffer, size_t length,
        uint16_t packet_id,
        QoS qos);

size_t
make_unsuback_message(
        uint8_t *buffer, size_t length,
        uint16_t packet_id);

size_t
make_pingreq_message(
        uint8_t *buffer, size_t length);

size_t
make_pingresp_message(
        uint8_t *buffer, size_t length);

size_t
make_disconnect_message(
        uint8_t *buffer, size_t length);

#endif //SMQTTC_MESSAGES_H
