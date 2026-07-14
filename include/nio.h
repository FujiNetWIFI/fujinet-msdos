/**
 * FujiNet NIO protocol helpers for MS-DOS clients.
 *
 * This is intentionally separate from the FujiNet/Atari-style
 * FUJICOM command set.  It speaks the clean fujinet-nio service protocol:
 * FujiBus + SLIP framing, NIO service device IDs, and versioned binary
 * device payloads.
 */

#ifndef _NIO_H
#define _NIO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <fuji_f5.h>

enum {
  NIO_DEVICEID_CLOCK   = 0x45,
  NIO_DEVICEID_DISK    = 0xFC,
  NIO_DEVICEID_NETWORK = 0xFD,
  NIO_DEVICEID_FILE    = 0xFE,
};

enum {
  NIO_STATUS_OK = 0,
  NIO_STATUS_DEVICE_NOT_FOUND = 1,
  NIO_STATUS_INVALID_REQUEST = 2,
  NIO_STATUS_DEVICE_BUSY = 3,
  NIO_STATUS_NOT_READY = 4,
  NIO_STATUS_IO_ERROR = 5,
  NIO_STATUS_TIMEOUT = 6,
  NIO_STATUS_INTERNAL_ERROR = 7,
  NIO_STATUS_UNSUPPORTED = 8,
};

enum {
  NIO_ERR_NONE = 0,
  NIO_ERR_SHORT_FRAME = 1,
  NIO_ERR_LENGTH_MISMATCH = 2,
  NIO_ERR_CHECKSUM = 3,
  NIO_ERR_DEVICE_COMMAND = 4,
  NIO_ERR_FIELDS = 5,
  NIO_ERR_EMPTY_STATUS = 6,
  NIO_ERR_REPLY_TOO_LARGE = 7,
  NIO_ERR_STATUS = 8,
  NIO_ERR_PAYLOAD = 9,
  NIO_ERR_TIMEOUT = 10,
  NIO_ERR_BUFFER_FULL = 11,
  NIO_ERR_UART = 12,
};

enum {
  NIO_DISK_VERSION = 1,
  NIO_DISK_CMD_MOUNT = 0x01,
  NIO_DISK_CMD_UNMOUNT = 0x02,
  NIO_DISK_CMD_READ_SECTOR = 0x03,
  NIO_DISK_CMD_WRITE_SECTOR = 0x04,
  NIO_DISK_CMD_INFO = 0x05,
  NIO_DISK_CMD_CLEAR_CHANGED = 0x06,
  NIO_DISK_CMD_CREATE = 0x07,
  NIO_DISK_CMD_READ_SECTORS = 0x08,
  NIO_DISK_CMD_WRITE_SECTORS = 0x09,
};

enum {
  NIO_DISK_INFO_INSERTED = 0x01,
  NIO_DISK_INFO_READONLY = 0x02,
  NIO_DISK_INFO_DIRTY = 0x04,
  NIO_DISK_INFO_CHANGED = 0x08,
  NIO_DISK_INFO_HAS_GEOMETRY = 0x10,
  NIO_DISK_INFO_HAS_LAST_ERROR = 0x20,
};

typedef struct {
  uint8_t status;
  uint16_t payload_length;
} nio_response_t;

typedef struct {
  uint8_t flags;
  uint8_t slot;
  uint8_t image_type;
  uint16_t sector_size;
  uint32_t sector_count;
  uint8_t last_error;
} nio_disk_info_t;

extern bool nio_call(uint8_t device, uint8_t command,
                     const void far *payload, uint16_t payload_length,
                     void far *reply, uint16_t reply_capacity,
                     nio_response_t far *response);

extern bool nio_disk_info(uint8_t slot, nio_disk_info_t far *info);
extern bool nio_disk_read_sector(uint8_t slot, uint32_t lba,
                                 void far *buffer, uint16_t buffer_length,
                                 uint16_t far *bytes_read);
extern bool nio_disk_read_sectors(uint8_t slot, uint32_t lba, uint16_t count,
                                  void far *buffer, uint16_t buffer_length,
                                  uint16_t far *bytes_read);
extern bool nio_disk_write_sector(uint8_t slot, uint32_t lba,
                                  const void far *buffer, uint16_t buffer_length,
                                  uint16_t far *bytes_written);
extern bool nio_disk_write_sectors(uint8_t slot, uint32_t lba, uint16_t count,
                                   const void far *buffer, uint16_t buffer_length,
                                   uint16_t far *bytes_written);
extern bool nio_status_ok(uint8_t status);
extern uint8_t nio_last_error;
extern uint8_t nio_last_status;
extern uint16_t nio_last_rx_len;
extern uint16_t nio_last_expected_len;
extern uint8_t nio_last_lsr;
extern uint16_t nio_network_timeout_ms;
extern uint8_t nio_transport_retries;

#endif /* _NIO_H */
