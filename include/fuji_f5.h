/**
 * low level FujiNet API
 */

#ifndef _FUJI_F5_H
#define _FUJI_F5_H

#include <stdint.h>

#define FUJINET_INT     0xF5
#define FUJIINT_NONE    0x00
#define FUJIINT_READ    0x40
#define FUJIINT_WRITE   0x80

#define FUJICOM_TIMEOUT  -1

enum {
  FUJI_FIELD_NONE        = 0,
  FUJI_FIELD_A1          = 1,
  FUJI_FIELD_A1_A2       = 2,
  FUJI_FIELD_A1_A2_A3    = 3,
  FUJI_FIELD_A1_A2_A3_A4 = 4,
  FUJI_FIELD_B12         = 5,
  FUJI_FIELD_B12_B34     = 6,
  FUJI_FIELD_C1234       = 7,
};

enum {
  FUJI_DEVICEID_FUJINET      = 0x70,

  FUJI_DEVICEID_DISK         = 0x31,
  FUJI_DEVICEID_DISK_LAST    = 0x3F,
  FUJI_DEVICEID_PRINTER      = 0x40,
  FUJI_DEVICEID_PRINTER_LAST = 0x43,
  FUJI_DEVICEID_VOICE        = 0x43,
  FUJI_DEVICEID_CLOCK        = 0x45,
  FUJI_DEVICEID_SERIAL       = 0x50,
  FUJI_DEVICEID_SERIAL_LAST  = 0x53,
  FUJI_DEVICEID_CPM          = 0x5A,
  FUJI_DEVICEID_NETWORK      = 0x71,
  FUJI_DEVICEID_NETWORK_LAST = 0x78,
  FUJI_DEVICEID_MIDI         = 0x99,
};

enum {
  FUJICMD_RENAME            = 0x20,
  FUJICMD_DELETE            = 0x21,
  FUJICMD_SEEK              = 0x25,
  FUJICMD_TELL              = 0x26,
  FUJICMD_MKDIR             = 0x2a,
  FUJICMD_RMDIR             = 0x2b,
  FUJICMD_CHDIR             = 0x2c,
  FUJICMD_GETCWD            = 0x30,
  FUJICMD_OPEN              = 'O',
  FUJICMD_CLOSE             = 'C',
  FUJICMD_READ              = 'R',
  FUJICMD_WRITE             = 'W',
  FUJICMD_STATUS            = 'S',
  FUJICMD_PARSE             = 'P',
  FUJICMD_QUERY             = 'Q',
  FUJICMD_APETIME_GETTIME   = 0x93,
  FUJICMD_APETIME_SETTZ     = 0x99,
  FUJICMD_APETIME_GETTZTIME = 0x9A,
  FUJICMD_MOUNT_ALL         = 0xD7,
  FUJICMD_GET_ADAPTERCONFIG = 0xE8,
  FUJICMD_UNMOUNT_IMAGE     = 0xE9,
  FUJICMD_READ_DEVICE_SLOTS = 0xF2,
  FUJICMD_MOUNT_IMAGE       = 0xF8,
  FUJICMD_MOUNT_HOST        = 0xF9,
  FUJICMD_JSON              = 0xFC,
  FUJICMD_USERNAME          = 0xFD,
  FUJICMD_PASSWORD          = 0xFE,
};

enum {
  SLOT_READONLY                 = 1,
  SLOT_READWRITE                = 2,
};

enum {
  NETWORK_SUCCESS                               = 1,
  NETWORK_ERROR_WRITE_ONLY                      = 131,
  NETWORK_ERROR_INVALID_COMMAND                 = 132,
  NETWORK_ERROR_READ_ONLY                       = 135,
  NETWORK_ERROR_END_OF_FILE                     = 136,
  NETWORK_ERROR_GENERAL_TIMEOUT                 = 138,
  NETWORK_ERROR_GENERAL                         = 144,
  NETWORK_ERROR_NOT_IMPLEMENTED                 = 146,
  NETWORK_ERROR_FILE_EXISTS                     = 151,
  NETWORK_ERROR_NO_SPACE_ON_DEVICE              = 162,
  NETWORK_ERROR_INVALID_DEVICESPEC              = 165,
  NETWORK_ERROR_ACCESS_DENIED                   = 167,
  NETWORK_ERROR_FILE_NOT_FOUND                  = 170,
  NETWORK_ERROR_CONNECTION_REFUSED              = 200,
  NETWORK_ERROR_NETWORK_UNREACHABLE             = 201,
  NETWORK_ERROR_SOCKET_TIMEOUT                  = 202,
  NETWORK_ERROR_NETWORK_DOWN                    = 203,
  NETWORK_ERROR_CONNECTION_RESET                = 204,
  NETWORK_ERROR_CONNECTION_ALREADY_IN_PROGRESS  = 205,
  NETWORK_ERROR_ADDRESS_IN_USE                  = 206,
  NETWORK_ERROR_NOT_CONNECTED                   = 207,
  NETWORK_ERROR_SERVER_NOT_RUNNING              = 208,
  NETWORK_ERROR_NO_CONNECTION_WAITING           = 209,
  NETWORK_ERROR_SERVICE_NOT_AVAILABLE           = 210,
  NETWORK_ERROR_CONNECTION_ABORTED              = 211,
  NETWORK_ERROR_INVALID_USERNAME_OR_PASSWORD    = 212,
  NETWORK_ERROR_COULD_NOT_PARSE_JSON            = 213,
  NETWORK_ERROR_CLIENT_GENERAL                  = 214,
  NETWORK_ERROR_SERVER_GENERAL                  = 215,
  NETWORK_ERROR_NO_DEVICE_AVAILABLE             = 216,
  NETWORK_ERROR_NOT_A_DIRECTORY                 = 217,
  NETWORK_ERROR_COULD_NOT_ALLOCATE_BUFFERS      = 255,
};

enum {
  REPLY_ERROR           = 'E',
  REPLY_COMPLETE        = 'C',
};

extern int fujiF5w(uint16_t descrdir, uint16_t devcom,
                  uint16_t aux12, uint16_t aux34, void far *buffer, uint16_t length);
#pragma aux fujiF5w = \
  "int 0xf5" \
  parm [dx] [ax] [cx] [si] [es bx] [di] \
  modify [ax]
#define fujiF5(dir, dev, cmd, descr, a12, a34, buf, len)         \
  fujiF5w(descr << 8 | dir, cmd << 8 | dev, a12, a34, buf, len)

#define fujiF5_none(d, c, fd, a12, a34, b, l) fujiF5(FUJIINT_NONE, d, c, fd, a12, a34, b, l)
#define fujiF5_read(d, c, fd, a12, a34, b, l) fujiF5(FUJIINT_READ, d, c, fd, a12, a34, b, l)
#define fujiF5_write(d, c, fd, a12, a34, b, l) fujiF5(FUJIINT_WRITE, d, c, fd, a12, a34, b, l)

#endif /* _FUJI_F5_H */
