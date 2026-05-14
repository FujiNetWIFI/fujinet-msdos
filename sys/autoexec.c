#include "fujinet.h"
#include "print.h"
#include "commands.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <dos.h>

#pragma data_seg("_CODE")

extern void int28_vect(void);

uint8_t fn_first_drive  = 0;
uint8_t fn_autoexec_armed = 0;
uint8_t fn_template_pos = 0;
void (__interrupt __far *fn_old_int28)(void);

static const char autoexec_cmd_template[] =
    "IF EXIST ?:\\CONFIG.EXE ?:\\CONFIG.EXE\r";

#define BDA_SEG             0x40
#define KB_HEAD_OFF         0x1A
#define KB_TAIL_OFF         0x1C
#define KB_BUF_START        0x1E
#define KB_BUF_END          0x3E

static int kb_buffer_empty(void)
{
    volatile uint16_t far *head = MK_FP(BDA_SEG, KB_HEAD_OFF);
    volatile uint16_t far *tail = MK_FP(BDA_SEG, KB_TAIL_OFF);
    return *head == *tail;
}

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

void int28_work(void)
{
    char ch;

    if (!fn_autoexec_armed)
        return;

    // On the very first fire, abort if the user already typed something.
    // Once we've started injecting, stay in our sequence regardless.
    if (fn_template_pos == 0 && !kb_buffer_empty()) {
        fn_autoexec_armed = 0;
        return;
    }

    while ((ch = autoexec_cmd_template[fn_template_pos]) != 0) {
        if (ch == '?')
            ch = fn_first_drive;
        if (!kb_stuff_char((uint8_t) ch))
            return;  // buffer full -- resume on next INT 28h
        fn_template_pos++;
    }

    fn_autoexec_armed = 0;
}

void setup_autoexec(uint8_t first_drive)
{
    if (!getenv("AUTOCFG"))
        return;

    fn_first_drive = first_drive;
    fn_autoexec_armed = 1;
    fn_old_int28 = _dos_getvect(0x28);
    _dos_setvect(0x28, MK_FP(getCS(), int28_vect));
    consolef("Auto-run armed for %c:\\CONFIG.EXE (slot 0 / autorun.img)\n",
             first_drive);
}
