#include "../fujicom/fujicom.h"

#warning "FIXME - use the real fujinet-fuji.h"

#define FILE_MAXLEN    36

typedef struct
{
  uint8_t hostSlot;
  uint8_t mode;
  uint8_t file[FILE_MAXLEN];
} DeviceSlot;

enum disk_access_flags_t {
  DISK_ACCESS_MODE_INVALID = 0x00,
  DISK_ACCESS_MODE_READ    = 0x01,
  DISK_ACCESS_MODE_WRITE   = 0x02,
  DISK_ACCESS_MODE_MOUNTED = 0x40,
};

#define fuji_get_device_slots(d, count) \
  fujiF5(FUJIINT_READ, FUJI_DEVICEID_FUJINET, FUJICMD_READ_DEVICE_SLOTS, \
         FUJI_FIELD_NONE, 0, 0, \
         d, sizeof(DeviceSlot) * count)
