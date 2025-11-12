// tlv_parser.h
#ifndef TLV_PARSER_H
#define TLV_PARSER_H

#include "protocol_spec.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    TLV_SEEKING_SOF,
    TLV_READING_HEADER,
    TLV_READING_PAYLOAD,
    TLV_READING_CRC
} tlv_state_t;

typedef struct
{
    tlv_state_t state;
    uint8_t buffer[TLV_MAX_LEN + 6]; // SOF(2) + max frame(32) + safety margin
    uint16_t buffer_len;
    uint16_t expected_len;
    uint16_t payload_len;
    uint8_t current_type;
    LaneCountsPayload current_payload;
} tlv_parser_t;

// Function prototypes
void tlv_parser_init (tlv_parser_t *parser);
bool tlv_parser_process_byte (tlv_parser_t *parser, uint8_t byte);
void tlv_parser_reset (tlv_parser_t *parser);

#endif
