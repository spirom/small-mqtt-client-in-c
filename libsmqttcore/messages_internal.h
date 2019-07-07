
#ifndef SMQTTC_MESSAGES_INTERNAL_H
#define SMQTTC_MESSAGES_INTERNAL_H

#include <stdbool.h>
#include <stdint.h>

#define MAX_TOPICS 32

/**
 * Encode the given size as one to four bytes of variable-length,
 * LSB-first encoding with continuation bits.
 * @param size Value to be encoded
 * @param buffer Encoding destination -- must have at least four bytes allocated
 * @param bytes_used The number of bytes to needed to encode the value, min 1, max 4.
 * @return Whether the value could successfully be encoded in the given buffer.
 */
bool
encode_size(size_t size, uint8_t *buffer, uint8_t *bytes_used);

/**
 * Decode up to the first four bytes of the given buffer as a size.
 * @param buffer Encoded bytes
 * @param buffer_end last bytes of the encoded bytes, which may be past
 *        the end of the size being decoded
 * @param size The decoded value
 * @param bytes_used Number of bytes used for decoding, min 1, max 4.
 * @return Whether a value could successfully be decoded from the given buffer.
 */
bool
decode_size(uint8_t *buffer, const uint8_t *buffer_end,
        size_t *size, uint8_t *bytes_used);

#endif //SMQTTC_MESSAGES_INTERNAL_H
