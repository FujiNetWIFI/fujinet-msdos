#include "../sys/ioctl.h"
#include "fujinet-fuji.h"
#include <stdio.h>
#include <string.h>
#include <i86.h>

#define FUJI_SIGNATURE "FUJI"
#define NUM_DEV_SLOTS 8

int find_driver_letter(int drive)
{
  union REGS regs;
  struct SREGS sregs;
  fuji_ioctl_query query;


  memset(&query, 0, sizeof(query));

  regs.h.ah = 0x44;       /* IOCTL Function */
  regs.h.al = 0x04;       /* Receive Control Data (Block) */
  regs.h.bl = (unsigned char)drive;
  regs.w.cx = sizeof(query);
  regs.x.dx = FP_OFF(&query);
  sregs.ds  = FP_SEG(&query);

  int86x(0x21, &regs, &regs, &sregs);

  /* If Carry Flag is NOT set, the call succeeded */
  if (!(regs.x.cflag & INTR_CF)) {
    if (memcmp(query.signature, FUJI_SIGNATURE, 4) == 0) {
      return query.unit; /* Match found! */
    }
  }


  return -1; /* Not found */
}

int main()
{
  int drive, fuji_unit;
  uint8_t rw;
  DeviceSlot dev_slots[NUM_DEV_SLOTS];


  if (!fuji_get_device_slots(dev_slots, NUM_DEV_SLOTS)) {
    printf("Unable to read device slots\n");
    return 1;
  }

  /* Start from 3 (C:), as A/B are usually floppy/standard bios */
  for (drive = 3; drive <= 26; drive++) {
    fuji_unit = find_driver_letter(drive);
    if (fuji_unit != -1) {
      rw = dev_slots[fuji_unit].mode & 0x3;
      if (dev_slots[fuji_unit].mode & DISK_ACCESS_MODE_MOUNTED)
        printf("\x16\x16\x16 ");
      else
        printf("\xC4\xFE\xC4 ");
      printf("%c:", 'A' + drive - 1);
      if (dev_slots[fuji_unit].hostSlot == 0xFF)
        printf(" -- no disk selected --");
      else
        printf(" %c %d:%s",
             rw == DISK_ACCESS_MODE_READ ? 'R' : (rw == DISK_ACCESS_MODE_WRITE ? 'W' : ' '),
             dev_slots[fuji_unit].hostSlot,
             dev_slots[fuji_unit].file);
      printf("\n");
    }
  }

  return 0;
}
