#include "commands.h"
#include <fuji_f5.h>
#include <fuji_firmware.h>
#include <dos.h>

#pragma data_seg("_CODE")

/*
 * NIO INT F5 extension:
 *   AX    = F502h
 *   ES:BX = fuji_ioctl_nio_call
 *   DI    = sizeof(fuji_ioctl_nio_call)
 *
 * Returns AX='C' on accepted control call, AX='E' on invalid request.
 * The NIO status and serial diagnostics are returned inside the control block.
 */
int intf5(uint16_t descrdir, uint16_t devcom, uint16_t aux12, uint16_t aux34,
          void far *ptr, uint16_t length)
#pragma aux intf5 parm [dx] [ax] [cx] [si] [es bx] [di] value [ax]
{
  fuji_ioctl_nio_call far *call;
  uint16_t status;

  (void) descrdir;
  (void) aux12;
  (void) aux34;

  _enable();

  if (devcom != FUJIF5_NIO_CALL_REQUEST ||
      ptr == 0 ||
      length < sizeof(fuji_ioctl_nio_call))
    return 'E';

  call = (fuji_ioctl_nio_call far *) ptr;
  status = nio_handle_control_call(call, 0);
  return (status & ERROR_BIT) ? 'E' : 'C';
}
