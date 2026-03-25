#include <dos.h>
#include <i86.h>
#include <fuji_f5.h>
#include <stdint.h>
#include "../sys/print.h"
#include "commands.h"

#pragma data_seg("_CODE")

/*
 * DL		== direction
 * AL		== device
 * AH		== command
 * CL		== aux1
 * CH		== aux2
 * SI           == aux34
 * ES:BX	== far buffer pointer
 * DI		== buffer length
 */
int int17(uint16_t direction, uint16_t cmdchar, uint16_t aux12, uint16_t aux34,
	  void far *ptr, uint16_t length)
#pragma aux int17 parm [dx] [ax] [cx] [si] [es bx] [di] value [ax]
{
    unsigned char ah=cmdchar >> 8;
    unsigned char al=cmdchar & 0xFF;

    _enable();

    switch (ah)
    {
    case 0:
        fujiF5_write(FUJI_DEVICEID_PRINTER, FUJICMD_WRITE, FUJI_FIELD_NONE, 0, 0, &al, 1);
        return 0;
    case 1:
        return 0;
    case 2:
        return 0;
    }

    return 0;
}

void set17(void)
{
    extern void int17_vect();

    _dos_setvect(0x17, MK_FP(getCS(), int17_vect));
}
