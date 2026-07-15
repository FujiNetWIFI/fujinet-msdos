#include "nio_disk_protocol.h"

uint16_t nio_disk_get_u16le(const uint8_t NIO_PROTO_PTR *p)
{
  return (uint16_t) p[0] | ((uint16_t) p[1] << 8);
}

uint32_t nio_disk_get_u32le(const uint8_t NIO_PROTO_PTR *p)
{
  return (uint32_t) p[0]
      | ((uint32_t) p[1] << 8)
      | ((uint32_t) p[2] << 16)
      | ((uint32_t) p[3] << 24);
}

void nio_disk_put_u16le(uint8_t NIO_PROTO_PTR *p, uint16_t v)
{
  p[0] = (uint8_t) (v & 0xFF);
  p[1] = (uint8_t) ((v >> 8) & 0xFF);
}

void nio_disk_put_u32le(uint8_t NIO_PROTO_PTR *p, uint32_t v)
{
  p[0] = (uint8_t) (v & 0xFF);
  p[1] = (uint8_t) ((v >> 8) & 0xFF);
  p[2] = (uint8_t) ((v >> 16) & 0xFF);
  p[3] = (uint8_t) ((v >> 24) & 0xFF);
}

uint16_t nio_disk_build_slot_request(uint8_t slot,
                                     uint8_t NIO_PROTO_PTR *req,
                                     uint16_t req_capacity)
{
  if (!req || req_capacity < NIO_DISK_SLOT_REQUEST_SIZE)
    return 0;
  req[0] = NIO_DISK_VERSION;
  req[1] = slot;
  return NIO_DISK_SLOT_REQUEST_SIZE;
}

uint16_t nio_disk_build_read_sector_request(uint8_t slot, uint32_t lba,
                                            uint16_t buffer_length,
                                            uint8_t NIO_PROTO_PTR *req,
                                            uint16_t req_capacity)
{
  if (!req || req_capacity < NIO_DISK_READ_SECTOR_REQUEST_SIZE)
    return 0;
  req[0] = NIO_DISK_VERSION;
  req[1] = slot;
  nio_disk_put_u32le(&req[2], lba);
  nio_disk_put_u16le(&req[6], buffer_length);
  return NIO_DISK_READ_SECTOR_REQUEST_SIZE;
}

uint16_t nio_disk_build_read_sectors_request(uint8_t slot, uint32_t lba,
                                             uint16_t count,
                                             uint16_t buffer_length,
                                             uint8_t NIO_PROTO_PTR *req,
                                             uint16_t req_capacity)
{
  if (!req || req_capacity < NIO_DISK_READ_SECTORS_REQUEST_SIZE)
    return 0;
  req[0] = NIO_DISK_VERSION;
  req[1] = slot;
  nio_disk_put_u32le(&req[2], lba);
  nio_disk_put_u16le(&req[6], count);
  nio_disk_put_u16le(&req[8], buffer_length);
  return NIO_DISK_READ_SECTORS_REQUEST_SIZE;
}

bool nio_disk_validate_response(const uint8_t NIO_PROTO_PTR *resp,
                                uint16_t payload_length,
                                uint16_t min_length)
{
  return resp && payload_length >= min_length && resp[0] == NIO_DISK_VERSION;
}

bool nio_disk_parse_info_response(const uint8_t NIO_PROTO_PTR *resp,
                                  uint16_t payload_length,
                                  nio_disk_info_t NIO_PROTO_PTR *info)
{
  if (!info || !nio_disk_validate_response(resp, payload_length,
                                           NIO_DISK_INFO_RESPONSE_MIN_SIZE))
    return false;

  info->flags = resp[1];
  info->slot = resp[4];
  info->image_type = resp[5];
  info->sector_size = nio_disk_get_u16le(&resp[6]);
  info->sector_count = nio_disk_get_u32le(&resp[8]);
  info->last_error = (payload_length > 12) ? resp[12] : 0;
  return true;
}

static bool parse_read_response(const uint8_t NIO_PROTO_PTR *resp,
                                uint16_t payload_length,
                                uint16_t min_length,
                                uint16_t length_offset,
                                uint16_t data_offset,
                                uint16_t buffer_length,
                                uint16_t NIO_PROTO_PTR *data_len)
{
  uint16_t len;

  if (!data_len || !nio_disk_validate_response(resp, payload_length, min_length))
    return false;

  len = nio_disk_get_u16le(&resp[length_offset]);
  if (len > buffer_length || payload_length < (uint16_t) (data_offset + len))
    return false;

  *data_len = len;
  return true;
}

bool nio_disk_parse_read_sector_response(const uint8_t NIO_PROTO_PTR *resp,
                                         uint16_t payload_length,
                                         uint16_t buffer_length,
                                         uint16_t NIO_PROTO_PTR *data_len)
{
  return parse_read_response(resp, payload_length,
                             NIO_DISK_READ_SECTOR_RESPONSE_MIN_SIZE,
                             9, 11, buffer_length, data_len);
}

bool nio_disk_parse_read_sectors_response(const uint8_t NIO_PROTO_PTR *resp,
                                          uint16_t payload_length,
                                          uint16_t buffer_length,
                                          uint16_t NIO_PROTO_PTR *data_len)
{
  return parse_read_response(resp, payload_length,
                             NIO_DISK_READ_SECTORS_RESPONSE_MIN_SIZE,
                             11, 13, buffer_length, data_len);
}
