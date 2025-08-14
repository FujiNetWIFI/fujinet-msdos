/**
 * @brief   FMALL - Mount All FujiNet device Slots
 * @author  Thomas Cherryhomes
 * @email   thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE.md for details.
 */

#include <stdio.h>
#include <i86.h>
#include <dos.h>

union REGS r;

int main(void)
{
    puts("Mounting All Slots...");

    r.h.dl = 0x00; // No payload
    r.h.ah = 0xD7; // Mount All
    r.h.al = 0x70; // Fuji device
    int86(0xF5,&r,&r);

    if (r.h.al != 'C')
        puts("Error!\n");
    else
        puts("Done.\n");

    return 0;
}
