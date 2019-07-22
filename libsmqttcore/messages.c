

#include <stdint.h>
#include <stddef.h>
#include <malloc.h>
#include <netinet/in.h>
#include <string.h>
#include <stdbool.h>

#include "messages.h"
#include "messages_internal.h"

#define PACK_MESSAGE_TYPE(M) ((uint8_t)((uint8_t)M << 4u))
#define UNPACK_MESSAGE_TYPE(M) (((uint8_t)M >> 4u))

#define BOOL_TO_BIT(b) ((b) ? 0x01u : 0x00u)

#define PACK_CONNECT_FLAGS(user, pw, will_flag, will_qos, will_retain, clean) \
    ( (BOOL_TO_BIT(user) << 7u) | (BOOL_TO_BIT(pw) << 6u) | \
      (BOOL_TO_BIT(will_retain) << 5u) | ((will_qos & 0x03u) << 3u) | \
      (BOOL_TO_BIT(will_flag) << 2u) | \
      (BOOL_TO_BIT(clean) << 1u) )

#define FLAGS_UNPACK_USER(bits) ((bool)((((uint8_t)bits) & 0x80u) >> 7u))
#define FLAGS_UNPACK_PW(bits) ((bool)((bits & 0x40u) >> 6u))
#define FLAGS_UNPACK_WILL_RETAIN(bits) ((bool)((bits & 0x20u) >> 5u))
#define FLAGS_UNPACK_WILL_QOS(bits) ((QoS)((bits & 0x18u) >> 3u))
#define FLAGS_UNPACK_WILL_FLAG(bits) ((bool)((bits & 0x04u) >> 2u))
#define FLAGS_UNPACK_CLEAN(bits) ((bool)((bits & 0x02u) >> 1u))

#define QOS_RETURN_OK(bits) ((bits & 0xFCu) == 0u)
#define QOS_RETURN_QOS(bits) ((QoS)(bits & 0x3u))

static uint16_t
short_from_buffer(const uint8_t *buffer)
{
    uint16_t net_short = (uint16_t)(buffer[0] << 16u) | (uint16_t)buffer[1];
    return net_short;
}

typedef struct cursor {
    uint8_t *base;
    uint8_t *current;
} cursor_t;

// TODO: add overflow checks

#define CURSOR_POSITION(cursor) (cursor.current - cursor.base)

#define CURSOR_INIT(cursor, origin) do { \
    cursor.base = cursor.current = origin; \
} while (false)

#define SER_byte(cursor, b) do {*(cursor.current++) = b;} while (false)

// for pulling apart a uint16_t
#define MSB(A) ((uint8_t)(((uint8_t)A & 0xFF00u) >> 8u))
#define LSB(A) ((uint8_t)((uint8_t)A & 0x00FFu))

#define SER_short(cursor, s) do { \
    *(cursor.current++) = MSB(s); \
    *(cursor.current++) = LSB(s); \
} while (false)

#define SER_string(cursor, str) do { \
    size_t len = strlen(str); \
    strncpy(cursor.current, str, len); \
    cursor.current += len; \
} while (false)

#define SER_len(cursor, len) do { \
    uint8_t used; \
    encode_size(len, cursor.current, &used); \
    cursor.current += used; \
} while (false)

static const uint16_t no_packet_id = 0U;

bool
encode_size(size_t size, uint8_t *buffer, uint8_t *bytes_used)
{
    uint8_t used = 0;
    size_t remaining_size = size;
    while (used < 4) {
        uint8_t digit = remaining_size % 128;
        remaining_size = remaining_size / 128;
        if (remaining_size > 0) {
            digit = digit | 0x80u;
        }
        buffer[used] = digit;
        used++;
        if (remaining_size == 0) break;
    }
    *bytes_used = used;
    if (remaining_size > 0) {
        return false;
    } else {
        return true;
    }
}


bool
decode_size(uint8_t *buffer, const uint8_t *buffer_end,
        size_t *size, uint8_t *bytes_used)
{
    size_t multiplier = 1;
    size_t value = 0;
    uint8_t used = 0;
    uint8_t digit = 0;
    while (buffer <= buffer_end && used < 4) {
        digit = *buffer;
        buffer++;
        value += (digit & 127u) * multiplier;
        multiplier *= 128;
        used++;
        if (! (digit & 0x80u)) break;
    }
    *bytes_used = used;
    if ((used == 4) && (digit & 0x80u)) {
        // incorrect encoding or not enough buffer
        return false;
    } else {
        *size = value;
        return true;
    }
}

/**
 * Parse all the UTF-8 strings out of 'buffer' to a max of 'bytecount' bytes and place their lengths
 * in 'lengths' and the (unterminated) strings in 'values'
 * @param buffer
 * @param bytecount
 * @param lengths
 * @param values
 * @return Number of extracted strings
 */
static uint16_t
fill_topic_arrays(const uint8_t *buffer, uint32_t bytecount,
                  uint16_t *lengths, char **values, QoS *qoss)
{
    const uint8_t *max = buffer + bytecount;
    const uint8_t *curr = buffer;
    int position = 0;
    while ((curr + 2) <= max && position < MAX_TOPICS) {
        // invariant: have space for a length, which could be 0
        // invariant: have space for the result
        uint16_t length = short_from_buffer(curr);
        lengths[position] = length;
        values[position] = (char *)curr + 2;
        if (qoss != NULL) {
            qoss[position] = *(curr + 2 + length);
        }
        position++;
        curr = curr + 2 + length;
        if (qoss != NULL) {
            curr++;
        }
    }
    return position;
}

static uint16_t
fill_subresult_arrays(const uint8_t *buffer, uint32_t bytecount,
                    bool *success, QoS *qoss)
{
    const uint8_t *max = buffer + bytecount;
    const uint8_t *curr = buffer;
    int position = 0;
    while ((curr + 1) <= max && position < MAX_TOPICS) {
        success[position] = QOS_RETURN_OK(*curr);
        qoss[position] = QOS_RETURN_QOS(*curr);
        position++;
        curr++;
    }
    return position;
}

/**
 * Caller is responsible for deleting response
 * @param buffer
 * @param length
 * @return
 */
response_t *
deserialize_response(uint8_t *buffer, size_t length)
{
    response_t *resp = malloc(sizeof(response_t));
    resp->type = UNPACK_MESSAGE_TYPE(buffer[0]);

    size_t total_size;
    uint8_t bytes_used;
    bool ret = decode_size(buffer + 1, buffer + length, &total_size, &bytes_used);
    if (!ret) {
        free(resp);
        return NULL;
    }

    size_t offset = 1 + bytes_used;

    switch (resp->type) {
        case CONNECT:
            resp->body.connect_data.name_size =
                    short_from_buffer(buffer + offset);
            offset += 2;
            resp->body.connect_data.name = buffer + offset;
            offset += resp->body.connect_data.name_size;
            resp->body.connect_data.version = buffer[offset];
            offset++;
            uint8_t connect_flags = buffer[offset];
            offset++;
            resp->body.connect_data.has_user = FLAGS_UNPACK_USER(connect_flags);
            resp->body.connect_data.has_pw = FLAGS_UNPACK_PW(connect_flags);
            resp->body.connect_data.has_will = FLAGS_UNPACK_WILL_FLAG(connect_flags);
            resp->body.connect_data.will_qos = FLAGS_UNPACK_WILL_QOS(connect_flags);
            resp->body.connect_data.will_retain = FLAGS_UNPACK_WILL_RETAIN(connect_flags);
            resp->body.connect_data.has_clean = FLAGS_UNPACK_CLEAN(connect_flags);
            resp->body.connect_data.keep_alive =
                    short_from_buffer(buffer + offset);
            offset += 2;
            resp->body.connect_data.client_id_size =
                    short_from_buffer(buffer + offset);
            offset += 2;
            resp->body.connect_data.client_id = buffer + offset;
            offset += resp->body.connect_data.client_id_size;
            if (resp->body.connect_data.has_will) {
                resp->body.connect_data.will_topic_size = short_from_buffer(buffer + offset);
                resp->body.connect_data.will_topic = buffer + offset + 2;
                offset += 2 + resp->body.connect_data.will_topic_size;
                resp->body.connect_data.will_message_size = short_from_buffer(buffer + offset);
                resp->body.connect_data.will_message = buffer + offset + 2;
                offset += 2 + resp->body.connect_data.will_message_size;
            }
            if (resp->body.connect_data.has_user) {
                resp->body.connect_data.user_size = short_from_buffer(buffer + offset);
                resp->body.connect_data.user = buffer + offset + 2;
                offset += 2 + resp->body.connect_data.user_size;
            }
            if (resp->body.connect_data.has_pw) {
                resp->body.connect_data.pw_size = short_from_buffer(buffer + offset);
                resp->body.connect_data.pw = buffer + offset + 2;
            }
            break;
        case CONNACK:
            resp->body.connack_data.type = buffer[offset + 1];
            break;
        case PUBLISH: {
            size_t saved_offset = offset;
            resp->body.publish_data.qos = (buffer[0] >> 1u) & 0x03u;
            resp->body.publish_data.retain = (buffer[0]) & 0x01u;

            resp->body.publish_data.topic_size = short_from_buffer(buffer + offset);
            offset += 2;
            resp->body.publish_data.topic = buffer + offset;
            offset += resp->body.publish_data.topic_size;
            resp->body.publish_data.packet_id = no_packet_id;
            if (resp->body.publish_data.qos >= QoS1) {
                resp->body.publish_data.packet_id =
                        short_from_buffer(buffer + offset);
                offset += 2;
            }
            resp->body.publish_data.message_size = total_size - (offset - saved_offset);
            resp->body.publish_data.message = buffer + offset;
            break;
        }
        case PUBACK:
            resp->body.puback_data.packet_id = short_from_buffer(buffer + offset);
            break;
        case PUBREC:
            resp->body.pubrec_data.packet_id = short_from_buffer(buffer + offset);
            break;
        case PUBREL:
            resp->body.pubrel_data.packet_id = short_from_buffer(buffer + offset);
            break;
        case PUBCOMP:
            resp->body.pubcomp_data.packet_id = short_from_buffer(buffer + offset);
            break;
        case SUBSCRIBE: {
            resp->body.subscribe_data.packet_id = short_from_buffer(buffer + offset);
            uint32_t topic_segment_size = total_size - offset;
            offset += 2;
            resp->body.subscribe_data.topic_count =
                    fill_topic_arrays(buffer + offset,
                                      topic_segment_size,
                                      resp->body.subscribe_data.topic_lengths,
                                      resp->body.subscribe_data.topics,
                                      resp->body.subscribe_data.qoss);
            break;
        }
        case SUBACK:
            resp->body.suback_data.packet_id = short_from_buffer(buffer + offset);
            uint32_t subresult_segment_size = total_size - offset;
            offset += 2;
            resp->body.suback_data.topic_count =
                    fill_subresult_arrays(
                            buffer + offset,
                            subresult_segment_size,
                            resp->body.suback_data.success,
                            resp->body.suback_data.qoss);
            break;
        case UNSUBSCRIBE:
            resp->body.unsubscribe_data.packet_id = short_from_buffer(buffer + offset);
            uint32_t topic_segment_size = total_size - offset;
            offset += 2;
            resp->body.unsubscribe_data.topic_count =
                    fill_topic_arrays(buffer + offset,
                                      topic_segment_size,
                                      resp->body.unsubscribe_data.topic_lengths,
                                      resp->body.unsubscribe_data.topics,
                                      NULL);
            break;
        case UNSUBACK:
            resp->body.unsuback_data.packet_id = short_from_buffer(buffer + offset);
            break;
        case PINGREQ:
            // no payload
            break;
        case PINGRESP:
            // no payload
            break;
        case DISCONNECT:
            // no payload
            break;
        default:
            // unknown message type
            free(resp);
            resp = NULL;
    }

    return resp;
}

size_t
make_connect_message(uint8_t *buffer, size_t length,
                     const char *clientid,
                     const char *user,
                     const char *pw,
                     uint8_t keep_alive_sec,
                     bool clean_session,
                     bool last_will_and_testament,
                     QoS will_qos,
                     bool will_retain,
                     const char *will_topic,
                     const char *will_message)
{
    static char *protocol_name = "MQIsdp";
    static uint8_t protocol_level = 3u;

    // just compute the lengths once, but for some we can't be sure yet
    // that we can compute them so set to 0
    size_t protocol_name_len = strlen(protocol_name);
    size_t client_id_len = strlen(clientid);
    size_t will_topic_len = 0;
    size_t will_message_len = 0;
    size_t user_len = 0;
    size_t pw_len = 0;

    cursor_t cursor;
    CURSOR_INIT(cursor, buffer);
    // fixed header
    SER_byte(cursor, PACK_MESSAGE_TYPE(CONNECT));
    size_t len = 8 + protocol_name_len + client_id_len;
    if (last_will_and_testament) {
        will_topic_len = strlen(will_topic);
        will_message_len = strlen(will_message);
        len += 2 + will_topic_len + 2 + will_message_len;
    }
    if (user != NULL) {
        user_len = strlen(user);
        len += 2 + user_len;
    }
    if (pw != NULL) {
        pw_len = strlen(pw);
        len += 2 + pw_len;
    }
    SER_len(cursor, len);
    // var header -- protocol name
    SER_short(cursor, protocol_name_len);
    SER_string(cursor, protocol_name);
    // var header -- protocol level
    SER_byte(cursor, protocol_level);
    uint8_t connect_flags =
            PACK_CONNECT_FLAGS((user != NULL), (pw != NULL),
                    last_will_and_testament, will_qos, will_retain, clean_session);
    SER_byte(cursor, connect_flags);
    SER_short(cursor, keep_alive_sec);
    // payload -- just client id for now
    SER_short(cursor, client_id_len);
    SER_string(cursor, clientid);
    if (last_will_and_testament) {
        SER_short(cursor, will_topic_len);
        SER_string(cursor, will_topic);
        SER_short(cursor, will_message_len);
        SER_string(cursor, will_message);
    }
    if (user != NULL) {
        SER_short(cursor, user_len);
        SER_string(cursor, user);
    }
    if (pw != NULL) {
        SER_short(cursor, pw_len);
        SER_string(cursor, pw);
    }

    size_t total_size = CURSOR_POSITION(cursor);
    return total_size;
}

size_t
make_connack_message(
        uint8_t *buffer, size_t length,
        connack_msg_t type)
{
    cursor_t cursor;
    CURSOR_INIT(cursor, buffer);
    SER_byte(cursor, PACK_MESSAGE_TYPE(CONNACK));
    SER_len(cursor, 2);
    SER_byte(cursor, 0);
    SER_byte(cursor, type);

    return CURSOR_POSITION(cursor);
}

size_t
make_disconnect_message(uint8_t *buffer, size_t length)
{
    cursor_t cursor;
    CURSOR_INIT(cursor, buffer);;
    SER_byte(cursor, PACK_MESSAGE_TYPE(DISCONNECT));
    SER_byte(cursor, 0);
    return CURSOR_POSITION(cursor);
}

size_t
make_publish_message(uint8_t *buffer, size_t length,
                     const char *topic,
                     uint16_t packet_id,
                     QoS qos,
                     bool retain,
                     const char *msg)
{
    cursor_t cursor;
    CURSOR_INIT(cursor, buffer);
    SER_byte(cursor, ( PACK_MESSAGE_TYPE(PUBLISH) | (qos << 1u) | (retain & 0x1u)) );
    size_t packet_id_len = 0;
    if (qos >= QoS1) {
        packet_id_len = 2;
    }
    uint16_t message_len = 0;
    if (msg != NULL) {
        message_len = strlen(msg);
    }
    SER_len(cursor, (2 + strlen(topic) + packet_id_len + message_len));
    SER_short(cursor, strlen(topic));
    SER_string(cursor, topic);
    if (qos >= QoS1) {
        SER_short(cursor, packet_id);
    }
    if (msg != NULL) {
        SER_string(cursor, msg);
    }

    return CURSOR_POSITION(cursor);
}

size_t
make_puback_message(
        uint8_t *buffer, size_t length,
        uint16_t packet_id)
{
    cursor_t cursor;
    CURSOR_INIT(cursor, buffer);
    SER_byte(cursor, PACK_MESSAGE_TYPE(PUBACK));
    SER_len(cursor, 2);
    SER_short(cursor, packet_id);

    return CURSOR_POSITION(cursor);
}

size_t
make_pubrec_message(
        uint8_t *buffer, size_t length,
        uint16_t packet_id)
{
    cursor_t cursor;
    CURSOR_INIT(cursor, buffer);
    SER_byte(cursor, PACK_MESSAGE_TYPE(PUBREC));
    SER_len(cursor, 2);
    SER_short(cursor, packet_id);

    return CURSOR_POSITION(cursor);
}

size_t
make_pubrel_message(
        uint8_t *buffer, size_t length,
        uint16_t packet_id)
{
    cursor_t cursor;
    CURSOR_INIT(cursor, buffer);
    SER_byte(cursor, PACK_MESSAGE_TYPE(PUBREL) | (QoS1 << 1u) );
    SER_len(cursor, 2);
    SER_short(cursor, packet_id);

    return CURSOR_POSITION(cursor);
}

size_t
make_pubcomp_message(
        uint8_t *buffer, size_t length,
        uint16_t packet_id)
{
    cursor_t cursor;
    CURSOR_INIT(cursor, buffer);
    SER_byte(cursor, PACK_MESSAGE_TYPE(PUBCOMP));
    SER_len(cursor, 2);
    SER_short(cursor, packet_id);

    return CURSOR_POSITION(cursor);
}


/**
 * Sum up UTF-8 tring lengths of a arrays of strings,
 * optionally including QoS settings.
 * @param topic_count
 * @param topics
 * @param include_qos add an extra byte for QoS on each topic?
 * @return
 */

static uint16_t
get_utf8_string_size(uint16_t topic_count, const char *topics[], bool include_qos)
{
    uint16_t total = 0;
    for (uint16_t i = 0; i < topic_count; i++) {
        total += 2 + strlen(topics[i]);
        if (include_qos) total++;
    }
    return total;
}

size_t
make_subscribe_message(uint8_t *buffer, size_t length,
                       uint16_t packet_id,
                       uint16_t topic_count,
                       const char *topics[],
                       const QoS qoss[])
{
    cursor_t cursor;
    CURSOR_INIT(cursor, buffer);
    // NOTE: qos below is independent of qos parameter, and always QoS1
    SER_byte(cursor, PACK_MESSAGE_TYPE(SUBSCRIBE) | (QoS1 << 1u) );
    SER_len(cursor, 2 + get_utf8_string_size(topic_count, topics, true));
    SER_short(cursor, packet_id);

    for (uint16_t position = 0; position < topic_count; position++) {
        SER_short(cursor, strlen(topics[position]));
        SER_string(cursor, topics[position]);
        SER_byte(cursor, qoss[position]);
    }

    return CURSOR_POSITION(cursor);
}

size_t
make_suback_message(
        uint8_t *buffer, size_t length,
        uint16_t packet_id,
        uint16_t topic_count,
        const bool success[],
        const QoS qoss[])
{
    cursor_t cursor;
    CURSOR_INIT(cursor, buffer);
    SER_byte(cursor, PACK_MESSAGE_TYPE(SUBACK));
    SER_len(cursor, 2 + topic_count);
    SER_short(cursor, packet_id);
    for (uint16_t i = 0; i < topic_count; i++) {
        SER_byte(cursor, success[i] ? qoss[i] : 0x80);
    }

    return CURSOR_POSITION(cursor);
}

size_t
make_unsubscribe_message(
        uint8_t *buffer, size_t length,
        uint16_t packet_id,
        uint16_t topic_count,
        const char *topics[])
{
    cursor_t cursor;
    CURSOR_INIT(cursor, buffer);
    // NOTE: qos below is independent of qos parameter, and always QoS1
    SER_byte(cursor, PACK_MESSAGE_TYPE(UNSUBSCRIBE) | (QoS1 << 1u) );
    SER_len(cursor, 2 + get_utf8_string_size(topic_count, topics, false));
    SER_short(cursor, packet_id);

    for (uint16_t i = 0; i < topic_count; i++) {
        SER_short(cursor, strlen(topics[i]));
        SER_string(cursor, topics[i]);
    }

    return CURSOR_POSITION(cursor);
}

size_t
make_unsuback_message(
        uint8_t *buffer, size_t length,
        uint16_t packet_id)
{
    cursor_t cursor;
    CURSOR_INIT(cursor, buffer);
    SER_byte(cursor, PACK_MESSAGE_TYPE(UNSUBACK));
    SER_len(cursor, 2u);
    SER_short(cursor, packet_id);

    return CURSOR_POSITION(cursor);
}

size_t
make_pingreq_message(
        uint8_t *buffer, size_t length)
{
    cursor_t cursor;
    CURSOR_INIT(cursor, buffer);
    SER_byte(cursor, PACK_MESSAGE_TYPE(PINGREQ));
    SER_len(cursor, 0u);

    return CURSOR_POSITION(cursor);
}

size_t
make_pingresp_message(
        uint8_t *buffer, size_t length)
{
    cursor_t cursor;
    CURSOR_INIT(cursor, buffer);
    SER_byte(cursor, PACK_MESSAGE_TYPE(PINGRESP));
    SER_len(cursor, 0u);

    return CURSOR_POSITION(cursor);
}