#include <dos.h>
#include <i86.h>
#include <fujicom.h>
#include <stdint.h>
#include "print.h"
#include "commands.h"

#pragma data_seg("_CODE")

/*
 * DL		== direction
 * DH           == field descriptor
 * AL		== device
 * AH		== command
 * CL		== aux1
 * CH		== aux2
 * SI           == aux34
 * ES:BX	== far buffer pointer
 * DI		== buffer length
 */
int intf5(uint16_t descrdir, uint16_t devcom, uint16_t aux12, uint16_t aux34,
	  void far *ptr, uint16_t length)
#pragma aux intf5 parm [dx] [ax] [cx] [si] [es bx] [di] value [ax]
{
  bool success;

  _enable();

  switch (descrdir & 0xFF) {
  case FUJIINT_NONE: // No Payload
    success = fuji_bus_call(devcom & 0xFF, devcom >> 8, descrdir >> 8,
                            aux12 & 0xFF, aux12 >> 8, aux34 & 0xFF, aux34 >> 8,
                            NULL, 0, NULL, 0);
    break;
  case FUJIINT_READ: // READ (Fujinet -> PC)
    success = fuji_bus_call(devcom & 0xFF, devcom >> 8, descrdir >> 8,
                            aux12 & 0xFF, aux12 >> 8, aux34 & 0xFF, aux34 >> 8,
                            NULL, 0, (uint8_t far *) ptr, length);
    break;
  case FUJIINT_WRITE: // WRITE (PC -> FujiNet)
    success = fuji_bus_call(devcom & 0xFF, devcom >> 8, descrdir >> 8,
                            aux12 & 0xFF, aux12 >> 8, aux34 & 0xFF, aux34 >> 8,
                            (uint8_t far *) ptr, length, NULL, 0);
    break;
  }

  return success ? 'C' : 'E';
}

void setf5(void)
{
    extern void intf5_vect();

    _dos_setvect(0xF5, MK_FP(getCS(), intf5_vect));
}
