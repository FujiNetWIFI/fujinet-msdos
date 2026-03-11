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

#define FUJICALL(cmd)                                                   \
  fuji_bus_call(FUJI_DEVICEID_FUJINET, cmd,                             \
                FUJI_FIELD_NONE,                                        \
                0, 0, 0, 0,                                             \
                NULL, 0, NULL, 0)
#define FUJICALL_RV(cmd, reply, replylen)                               \
  fuji_bus_call(FUJI_DEVICEID_FUJINET, cmd,                             \
                FUJI_FIELD_NONE,                                        \
		0, 0, 0, 0,                                             \
                NULL, 0, reply, replylen)
#define FUJICALL_A1(cmd, a1)                                            \
  fuji_bus_call(FUJI_DEVICEID_FUJINET, cmd,                             \
                FUJI_FIELD_A1,                                          \
                a1, 0, 0, 0,                                            \
                NULL, 0, NULL, 0)
#define FUJICALL_A1_A2(cmd, a1, a2)                                     \
  fuji_bus_call(FUJI_DEVICEID_FUJINET, cmd,                             \
                FUJI_FIELD_A1_A2,                                       \
                a1, a2, 0, 0,                                           \
                NULL, 0, NULL, 0)

#define fuji_get_device_slots(d, count) \
  FUJICALL_RV(FUJICMD_READ_DEVICE_SLOTS, d, sizeof(DeviceSlot) * count)
#define fuji_mount_all() \
  FUJICALL(FUJICMD_MOUNT_ALL)
#define fuji_mount_disk_image(ds, mode) \
  FUJICALL_A1_A2(FUJICMD_MOUNT_IMAGE, ds, mode)
#define fuji_mount_host_slot(hs) \
  FUJICALL_A1(FUJICMD_MOUNT_HOST, hs)
#define fuji_unmount_disk_image(ds) \
  FUJICALL_A1(FUJICMD_UNMOUNT_IMAGE, ds)
