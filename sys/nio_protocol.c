#include "nio_protocol.h"

uint16_t nio_protocol_checksum(const void NIO_PROTO_PTR *ptr, uint16_t len, uint16_t seed)
{
  uint16_t idx, chk;
  const uint8_t NIO_PROTO_PTR *buf = (const uint8_t NIO_PROTO_PTR *) ptr;

  for (idx = 0, chk = seed; idx < len; idx++)
    chk = ((chk + buf[idx]) >> 8) + ((chk + buf[idx]) & 0xFF);
  return chk;
}

uint8_t nio_protocol_validate_response(const nio_frame_header_t NIO_PROTO_PTR *header,
                                       const uint8_t NIO_PROTO_PTR *payload,
                                       uint16_t rx_len,
                                       uint8_t expected_device,
                                       uint8_t expected_command,
                                       uint16_t reply_capacity,
                                       nio_parsed_response_t NIO_PROTO_PTR *parsed)
{
  nio_frame_header_t checksum_header;
  uint16_t payload_len;
  uint8_t checksum;

  if (parsed) {
    parsed->status = 0;
    parsed->expected_len = NIO_FRAME_HEADER_SIZE;
    parsed->payload_offset = 0;
    parsed->payload_length = 0;
  }

  if (rx_len < NIO_FRAME_HEADER_SIZE)
    return NIO_PROTO_ERR_SHORT_FRAME;

  if (parsed)
    parsed->expected_len = header->length;
  if (rx_len != header->length)
    return NIO_PROTO_ERR_LENGTH_MISMATCH;

  payload_len = (uint16_t) (rx_len - NIO_FRAME_HEADER_SIZE);
  checksum = header->checksum;
  checksum_header = *header;
  checksum_header.checksum = 0;
  if ((uint8_t) nio_protocol_checksum(payload, payload_len,
        nio_protocol_checksum(&checksum_header, sizeof(checksum_header), 0)) != checksum)
    return NIO_PROTO_ERR_CHECKSUM;

  if (header->device != expected_device || header->command != expected_command)
    return NIO_PROTO_ERR_DEVICE_COMMAND;

  if ((header->fields & 0x80) || (header->fields & 0x07) != NIO_PROTO_FIELD_A1)
    return NIO_PROTO_ERR_FIELDS;
  if (payload_len < 1)
    return NIO_PROTO_ERR_EMPTY_STATUS;

  if (payload_len - 1 > reply_capacity)
    return NIO_PROTO_ERR_REPLY_TOO_LARGE;

  if (parsed) {
    parsed->status = payload[0];
    parsed->payload_offset = 1;
    parsed->payload_length = (uint16_t) (payload_len - 1);
  }

  return NIO_PROTO_OK;
}
