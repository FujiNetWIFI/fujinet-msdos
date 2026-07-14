#include "commands.h"
#include <fuji_f5.h>
#include <dos.h>

void setf5(void)
{
  extern void intf5_vect(void);

  _dos_setvect(FUJIF5_INT, MK_FP(getCS(), intf5_vect));
}
