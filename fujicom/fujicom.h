/**
 * low level FujiNet API
 */

#ifndef _FUJICOM_H
#define _FUJICOM_H

#undef FUJIF5_AS_FUNCTION

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

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

#define U32_MSW(v) ((uint16_t)(((uint32_t)(v) >> 16) & 0xFFFF))  // Most Significant Word
#define U32_LSW(v) ((uint16_t)((uint32_t)(v) & 0xFFFF))          // Least Significant Word

#define U16_MSB(w) ((uint8_t)(((uint16_t)(w) >> 8) & 0xFF))
#define U16_LSB(w) ((uint8_t)((uint16_t)(w) & 0xFF))

typedef struct {
  uint16_t bw;
  uint8_t connected; /* meaning of this field is inconsistent */
  uint8_t error;
} fujiStatus;

typedef struct {
  unsigned char hostSlot;
  unsigned char mode;
  char file[36];
} deviceSlot_t;

// FIXME - get this from lib/device/sio/fuji.h
#define MAX_SSID_LEN 32
typedef struct {
  char ssid[MAX_SSID_LEN+1];
  char hostname[64];
  unsigned char localIP[4];
  unsigned char gateway[4];
  unsigned char netmask[4];
  unsigned char dnsIP[4];
  unsigned char macAddress[6];
  unsigned char bssid[6];
  char fn_version[15];
} AdapterConfig;

#pragma pack(pop)

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

#define STATUS_MOUNT_TIME       0x01

/**
 * @brief start fujicom
 */
extern void fujicom_init(void);

extern bool fuji_bus_call(uint8_t device, uint8_t fuji_cmd, uint8_t fields,
                          uint8_t aux1, uint8_t aux2, uint8_t aux3, uint8_t aux4,
                          const void far *data, size_t data_length,
                          void far *reply, size_t reply_length);

/**
 * @brief end fujicom
 */
void fujicom_done(void);

#ifndef FUJIF5_AS_FUNCTION
extern int fujiF5w(uint16_t descrdir, uint16_t devcom,
                  uint16_t aux12, uint16_t aux34, void far *buffer, uint16_t length);
#pragma aux fujiF5w = \
  "int 0xf5" \
  parm [dx] [ax] [cx] [si] [es bx] [di] \
  modify [ax]
#define fujiF5(dir, dev, cmd, descr, a12, a34, buf, len)         \
  fujiF5w(descr << 8 | dir, cmd << 8 | dev, a12, a34, buf, len)
#else
extern int fujiF5(uint8_t direction, uint8_t device, uint8_t command, uint8_t descr,
                  uint16_t aux12, uint16_t aux34, void far *buffer, uint16_t length);
#endif

#define fujiF5_none(d, c, fd, a12, a34, b, l) fujiF5(FUJIINT_NONE, d, c, fd, a12, a34, b, l)
#define fujiF5_read(d, c, fd, a12, a34, b, l) fujiF5(FUJIINT_READ, d, c, fd, a12, a34, b, l)
#define fujiF5_write(d, c, fd, a12, a34, b, l) fujiF5(FUJIINT_WRITE, d, c, fd, a12, a34, b, l)

#endif /* _FUJICOM_H */
