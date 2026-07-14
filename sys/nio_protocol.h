#ifndef FUJINET_MSDOS_NIO_PROTOCOL_H
#define FUJINET_MSDOS_NIO_PROTOCOL_H

#include <stdint.h>

#ifdef __WATCOMC__
#define NIO_PROTO_PTR far
#else
#define NIO_PROTO_PTR
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define NIO_FRAME_HEADER_SIZE 6

enum {
  NIO_PROTO_FIELD_NONE = 0,
  NIO_PROTO_FIELD_A1   = 1
};

enum {
  NIO_PROTO_OK = 0,
  NIO_PROTO_ERR_SHORT_FRAME = 1,
  NIO_PROTO_ERR_LENGTH_MISMATCH = 2,
  NIO_PROTO_ERR_CHECKSUM = 3,
  NIO_PROTO_ERR_DEVICE_COMMAND = 4,
  NIO_PROTO_ERR_FIELDS = 5,
  NIO_PROTO_ERR_EMPTY_STATUS = 6,
  NIO_PROTO_ERR_REPLY_TOO_LARGE = 7
};

typedef struct {
  uint8_t device;
  uint8_t command;
  uint16_t length;
  uint8_t checksum;
  uint8_t fields;
} nio_frame_header_t;

typedef struct {
  uint8_t status;
  uint16_t expected_len;
  uint16_t payload_offset;
  uint16_t payload_length;
} nio_parsed_response_t;

uint16_t nio_protocol_checksum(const void NIO_PROTO_PTR *ptr,
                               uint16_t len, uint16_t seed);

uint8_t nio_protocol_validate_response(const nio_frame_header_t NIO_PROTO_PTR *header,
                                       const uint8_t NIO_PROTO_PTR *payload,
                                       uint16_t rx_len,
                                       uint8_t expected_device,
                                       uint8_t expected_command,
                                       uint16_t reply_capacity,
                                       nio_parsed_response_t NIO_PROTO_PTR *parsed);

#ifdef __cplusplus
}
#endif

#endif
