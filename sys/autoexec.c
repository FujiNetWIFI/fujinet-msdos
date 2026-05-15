#include "fujinet.h"
#include "print.h"
#include "commands.h"
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <dos.h>

#pragma data_seg("_CODE")

extern void int21_vect(void);

uint8_t fn_first_drive  = 0;
uint8_t fn_autoexec_armed = 0;
void (__interrupt __far *fn_old_int21)(void);

static const char autoexec_cmd_template[] =
    "IF EXIST ?:\\CONFIG.EXE ?:\\CONFIG.EXE\r";

#define BDA_SEG             0x40
#define KB_HEAD_OFF         0x1A
#define KB_TAIL_OFF         0x1C
#define KB_BUF_START        0x1E
#define KB_BUF_END          0x3E

static int kb_stuff_char(uint8_t ch)
{
    volatile uint16_t far *head = MK_FP(BDA_SEG, KB_HEAD_OFF);
    volatile uint16_t far *tail = MK_FP(BDA_SEG, KB_TAIL_OFF);
    uint16_t t = *tail;
    uint16_t next = t + 2;
    uint8_t far *slot;

    if (next >= KB_BUF_END)
        next = KB_BUF_START;
    if (next == *head)
        return 0;

    slot = MK_FP(BDA_SEG, t);
    slot[0] = ch;
    slot[1] = 0;
    *tail = next;
    return 1;
}

void int21_inject(void)
{
    const char *p = autoexec_cmd_template;
    char ch;

    for (; (ch = *p) != 0; p++) {
        if (ch == '?')
            ch = fn_first_drive;
        if (!kb_stuff_char((uint8_t) ch))
            break;
    }
    fn_autoexec_armed = 0;
}

void setup_autoexec(uint8_t first_drive)
{
    if (!getenv("AUTOCFG"))
        return;

    fn_first_drive = first_drive;
    fn_autoexec_armed = 1;
    fn_old_int21 = _dos_getvect(0x21);
    _dos_setvect(0x21, MK_FP(getCS(), int21_vect));
    consolef("Auto-run armed for %c:\\CONFIG.EXE (slot 0 / autorun.img)\n",
             first_drive);
}
