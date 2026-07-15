#ifndef FUJINET_MSDOS_NIO_DISK_PROTOCOL_H
#define FUJINET_MSDOS_NIO_DISK_PROTOCOL_H

#include "nio_protocol.h"
#include "../include/nio.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
  NIO_DISK_SLOT_REQUEST_SIZE = 2,
  NIO_DISK_READ_SECTOR_REQUEST_SIZE = 8,
  NIO_DISK_READ_SECTORS_REQUEST_SIZE = 10,
  NIO_DISK_INFO_RESPONSE_MIN_SIZE = 12,
  NIO_DISK_CLEAR_CHANGED_RESPONSE_MIN_SIZE = 5,
  NIO_DISK_READ_SECTOR_RESPONSE_MIN_SIZE = 11,
  NIO_DISK_READ_SECTORS_RESPONSE_MIN_SIZE = 13
};

uint16_t nio_disk_get_u16le(const uint8_t NIO_PROTO_PTR *p);
uint32_t nio_disk_get_u32le(const uint8_t NIO_PROTO_PTR *p);
void nio_disk_put_u16le(uint8_t NIO_PROTO_PTR *p, uint16_t v);
void nio_disk_put_u32le(uint8_t NIO_PROTO_PTR *p, uint32_t v);

uint16_t nio_disk_build_slot_request(uint8_t slot,
                                     uint8_t NIO_PROTO_PTR *req,
                                     uint16_t req_capacity);
uint16_t nio_disk_build_read_sector_request(uint8_t slot, uint32_t lba,
                                            uint16_t buffer_length,
                                            uint8_t NIO_PROTO_PTR *req,
                                            uint16_t req_capacity);
uint16_t nio_disk_build_read_sectors_request(uint8_t slot, uint32_t lba,
                                             uint16_t count,
                                             uint16_t buffer_length,
                                             uint8_t NIO_PROTO_PTR *req,
                                             uint16_t req_capacity);

bool nio_disk_validate_response(const uint8_t NIO_PROTO_PTR *resp,
                                uint16_t payload_length,
                                uint16_t min_length);
bool nio_disk_parse_info_response(const uint8_t NIO_PROTO_PTR *resp,
                                  uint16_t payload_length,
                                  nio_disk_info_t NIO_PROTO_PTR *info);
bool nio_disk_parse_read_sector_response(const uint8_t NIO_PROTO_PTR *resp,
                                         uint16_t payload_length,
                                         uint16_t buffer_length,
                                         uint16_t NIO_PROTO_PTR *data_len);
bool nio_disk_parse_read_sectors_response(const uint8_t NIO_PROTO_PTR *resp,
                                          uint16_t payload_length,
                                          uint16_t buffer_length,
                                          uint16_t NIO_PROTO_PTR *data_len);

#ifdef __cplusplus
}
#endif

#endif /* FUJINET_MSDOS_NIO_DISK_PROTOCOL_H */
