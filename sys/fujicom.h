/**
 * low level FujiNet API
 */

#ifndef _FUJICOM_H
#define _FUJICOM_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

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

#endif /* _FUJICOM_H */
