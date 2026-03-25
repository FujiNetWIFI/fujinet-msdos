#include "../sys/ioctl.h"
#include "fujinet-fuji.h"
#include "getargs.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <i86.h>
#include <stdlib.h>

#define FUJI_SIGNATURE "FUJI"
#define NUM_DEV_SLOTS 8
#define NO_HOST 0xFF

#define ICON_DRIVE_EJECTED   "   "
#define ICON_DRIVE_MOUNTED   "\x16\x16\x16"
#define ICON_DRIVE_UNMOUNTED "\xC4\xFE\xC4"

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

int find_slot_drive(int slot)
{
  int drive;

  for (drive = 3; drive <= 26; drive++)
    if (find_driver_letter(drive) == slot)
      return drive;
  return -1;
}

int main(int argc, char *argv[])
{
  int count, idx;
  int drive, fuji_unit;
  uint8_t rw;
  DeviceSlot dev_slots[NUM_DEV_SLOTS];
  int mount_all = 0, eject = 0, mount_write = 0, tell_slot = -1;


  count = getargs(argc, argv, "abebwbti", &mount_all, &eject, &mount_write, &tell_slot);
  if (count < 0) {
    if (count < 0 && -count != '-')
      fprintf(stderr, "Bad flag: %c\n", -count);
    fprintf(stderr, "Usage: %s [-options] [drive:...]\n"
            "  options:\n"
            "\ta: mount all\n"
            "\te: eject listed drives, forgetting the disk image path\n"
            "\tw: mount disk as read/write\n"
            "\tt N: show DOS drive letter for FujiNet slot N\n"
            , *argv);
    exit(1);
  }

  if (!fuji_get_device_slots(dev_slots, NUM_DEV_SLOTS)) {
    printf("Unable to read device slots\n");
    return 1;
  }

  if (tell_slot >= 0) {
    drive = find_slot_drive(tell_slot);
    if (drive == -1)
      printf("No DOS drive mapped to FujiNet slot %d\n", tell_slot);
    else
      printf("%c:\n", 'A' + drive - 1);
    return 0;
  }

  if (mount_all) {
    // FIXME - complain if any args are passed?
    fuji_mount_all();
  }
  else if (argc - count == 0) {
    /* Start from 3 (C:), as A/B are usually floppy/standard bios */
    for (drive = 3; drive <= 26; drive++) {
      fuji_unit = find_driver_letter(drive);
      if (fuji_unit != -1) {
        rw = dev_slots[fuji_unit].mode & 0x3;
        if (dev_slots[fuji_unit].mode & DISK_ACCESS_MODE_MOUNTED)
          printf(ICON_DRIVE_MOUNTED);
        else if (dev_slots[fuji_unit].hostSlot == NO_HOST)
          printf(ICON_DRIVE_EJECTED);
        else
          printf(ICON_DRIVE_UNMOUNTED);
        printf(" %c:", 'A' + drive - 1);
        if (dev_slots[fuji_unit].hostSlot == NO_HOST)
          printf(" -- no disk selected --");
        else
          printf(" %c %d:%s",
                 rw == DISK_ACCESS_MODE_READ ? 'R' : (rw == DISK_ACCESS_MODE_WRITE ? 'W' : ' '),
                 dev_slots[fuji_unit].hostSlot,
                 dev_slots[fuji_unit].file);
        printf("\n");
      }
    }
  }
  else {
    for (idx = count; idx < argc; idx++) {
      drive = toupper(argv[idx][0]);
      if (drive < 'A' || drive > 'Z') {
        printf("Invalid drive letter: %c:\n", drive);
        exit(1);
      }
      fuji_unit = find_driver_letter(drive - 'A' + 1);
      if (fuji_unit == -1) {
        printf("Drive %c: is not a FujiNet drive\n", drive);
        exit(1);
      }
      if (eject) {
        if (!fuji_unmount_disk_image(fuji_unit)) {
          printf("Failed to eject %c:\n", drive);
          exit(1);
        }
        printf("Ejected %c:\n", drive);
      }
      else {
        if (dev_slots[fuji_unit].hostSlot == NO_HOST) {
          printf("No disk selected on drive %c:\n", drive);
          exit(1);
        }

        // FIXME - is there a URL in drive argument? Create host and add filename on device?

        if (!fuji_mount_host_slot(dev_slots[fuji_unit].hostSlot)) {
          printf("Failed to mount host for %c:\n", drive);
          exit(1);
        }
        if (!fuji_mount_disk_image(fuji_unit,
                                   mount_write ?
                                   DISK_ACCESS_MODE_WRITE : DISK_ACCESS_MODE_READ)) {
          printf("Failed to mount drive %c:\n", drive);
          exit(1);
        }
        printf("Mounted \"%s\" on %c:\n", dev_slots[fuji_unit].file, drive);
      }
    }
  }

  return 0;
}
