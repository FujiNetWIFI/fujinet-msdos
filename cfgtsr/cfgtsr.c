/**
 * @brief   CFGTSR - FujiNet autorun config TSR
 * @license gpl v. 3, see LICENSE.md for details.
 *
 * Pops up a small menu on Ctrl+Shift+F5 letting the user:
 *   M)ount the autorun.img bootdisk on FujiNet slot 0
 *   R)un CONFIG.EXE from that disk
 *   ESC cancel
 *
 * Usage:
 *   cfgtsr        run menu now (transient mode)
 *   cfgtsr /I     install TSR
 *   cfgtsr /U     uninstall TSR
 */

#include <conio.h>
#include <ctype.h>
#include <dos.h>
#include <i86.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fuji_f5.h"

/* ---- Multiplex protocol ---- */

#define MUX_ID          0xCF
#define MUX_INSTALLED   0xFF
#define MUX_CHECK       0x00
#define MUX_GET_PSP     0x01
#define MUX_UNINSTALL   0x02

/* ---- Hotkey: Ctrl+Alt+M ---- */

#define HOTKEY_SCAN     0x32   /* M make-code */

/* ---- Firmware commands ---- */
/* FUJI_DEVICEID_FUJINET comes from fuji_f5.h */

#define FUJICMD_SET_BOOT_MODE  0xD6

/* ---- BIOS data area ---- */

#define BDA_SEG    0x0040
#define KB_FLAGS   0x0017
#define KB_HEAD    0x001A
#define KB_TAIL    0x001C
#define KB_BUF_S   0x001E
#define KB_BUF_E   0x003E

/* ---- Popup geometry ---- */

#define VID_SEG       0xB800
#define POPUP_ROW     10
#define POPUP_COL     30
#define POPUP_WIDTH   20
#define POPUP_HEIGHT  5

/* ============================================================
   RESIDENT STATE
   ============================================================ */

extern unsigned _psp;

void (__interrupt __far *old_int9h)(void);
void (__interrupt __far *old_int28h)(void);
void (__interrupt __far *old_int2fh)(void);

volatile unsigned char popup_pending = 0;
unsigned char first_drive  = 0;
unsigned char stuffing     = 0;
unsigned char stuff_pos    = 0;

unsigned char screen_save[POPUP_HEIGHT * POPUP_WIDTH * 2];

char run_template[] = "?:\\CONFIG.EXE\r";

/* ============================================================
   RESIDENT CODE
   ============================================================ */

/* ---- BIOS keyboard buffer stuffing (same trick as autoexec.c) ---- */

int kb_stuff_char(unsigned char ch)
{
    unsigned int far *h = MK_FP(BDA_SEG, KB_HEAD);
    unsigned int far *t = MK_FP(BDA_SEG, KB_TAIL);
    unsigned int tail = *t;
    unsigned int next = tail + 2;
    unsigned char far *slot;

    if (next >= KB_BUF_E) next = KB_BUF_S;
    if (next == *h) return 0;

    slot = MK_FP(BDA_SEG, tail);
    slot[0] = ch;
    slot[1] = 0;
    *t = next;
    return 1;
}

void run_stuff_step(void)
{
    unsigned char ch;
    while ((ch = run_template[stuff_pos]) != 0) {
        if (ch == '?') ch = first_drive;
        if (!kb_stuff_char(ch))
            return;
        stuff_pos++;
    }
    stuffing = 0;
}

/* ---- Popup drawing ---- */

void save_screen(void)
{
    unsigned char far *vid;
    int row, i = 0;
    for (row = 0; row < POPUP_HEIGHT; row++) {
        int col;
        vid = MK_FP(VID_SEG, ((POPUP_ROW + row) * 80 + POPUP_COL) * 2);
        for (col = 0; col < POPUP_WIDTH * 2; col++)
            screen_save[i++] = vid[col];
    }
}

void restore_screen(void)
{
    unsigned char far *vid;
    int row, i = 0;
    for (row = 0; row < POPUP_HEIGHT; row++) {
        int col;
        vid = MK_FP(VID_SEG, ((POPUP_ROW + row) * 80 + POPUP_COL) * 2);
        for (col = 0; col < POPUP_WIDTH * 2; col++)
            vid[col] = screen_save[i++];
    }
}

void draw_popup(void)
{
    /* CP437 single-line box characters, matching fujinet-config */
    static const char lines[POPUP_HEIGHT][POPUP_WIDTH + 1] = {
        "\xDA\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xBF",
        "\xB3 M) Mount         \xB3",
        "\xB3 R) Run Config    \xB3",
        "\xB3 ESC) Cancel      \xB3",
        "\xC0\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xD9",
    };
    int row;
    for (row = 0; row < POPUP_HEIGHT; row++) {
        unsigned char far *vid =
            MK_FP(VID_SEG, ((POPUP_ROW + row) * 80 + POPUP_COL) * 2);
        const char *s = lines[row];
        int col;
        for (col = 0; col < POPUP_WIDTH; col++) {
            unsigned char c = (unsigned char) s[col];
            unsigned char attr = (c >= 0xB3 && c <= 0xDA)
                                 ? 0x1F   /* bright white on blue: border */
                                 : 0x17;  /* white on blue: body         */
            vid[col*2]   = c;
            vid[col*2+1] = attr;
        }
    }
}

unsigned char read_key(void)
{
    unsigned char ascii;
    /* Inline asm bypasses int86() library to rule it out as a culprit.
       AH=0 selects "wait for key" -- this MUST block until a key is pressed. */
    _asm {
        xor   ah, ah
        int   16h
        mov   ascii, al
    }
    return ascii;
}

/* ---- Firmware: set boot mode ---- */

int send_boot_mode(unsigned char mode)
{
    /* Boot mode is passed as aux1 (firmware reads packet.param(0)).
       FUJI_FIELD_A1 tells fuji_bus_call to include 1 aux byte in the
       packet. fujiF5w (#pragma aux in fuji_f5.h) generates inline
       `int 0xf5` -- no C library indirection. */
    return fujiF5_none(FUJI_DEVICEID_FUJINET, FUJICMD_SET_BOOT_MODE,
                       FUJI_FIELD_A1, mode, 0, NULL, 0) == 'C';
}

void do_popup(void)
{
    unsigned char ch;

    save_screen();
    draw_popup();
    ch = read_key();
    restore_screen();

    switch (toupper(ch)) {
    case 'M':
        send_boot_mode(0);
        break;
    case 'R':
        send_boot_mode(0);
        stuff_pos = 0;
        stuffing  = 1;
        break;
    default:
        break;
    }
}

/* ---- INT 9h: hotkey detector ---- */

void __interrupt __far __loadds my_int9h(void)
{
    unsigned char sc;
    unsigned char flags;
    unsigned char far *bda;

    sc = inp(0x60);

    if (sc == HOTKEY_SCAN) {
        bda   = MK_FP(BDA_SEG, KB_FLAGS);
        flags = *bda;
        if ((flags & 0x04) && (flags & 0x08)) {  /* Ctrl + Alt */
            unsigned char p61;
            popup_pending = 1;
            /* Pulse port 61h bit 7 to acknowledge keyboard */
            p61 = inp(0x61);
            outp(0x61, p61 | 0x80);
            outp(0x61, p61);
            /* EOI to PIC */
            outp(0x20, 0x20);
            return;  /* swallow */
        }
    }

    _chain_intr(old_int9h);
}

/* ---- INT 28h: deferred work ---- */

void __interrupt __far __loadds my_int28h(void)
{
    if (popup_pending) {
        popup_pending = 0;
        do_popup();
    }
    if (stuffing) {
        run_stuff_step();
    }
    _chain_intr(old_int28h);
}

/* ---- INT 2Fh multiplex ----
 *
 * The struct mirrors the Watcom 16-bit __interrupt prologue's stack layout.
 * Verified against `wdis cfgtsr.o`:  the prologue pushes ax,cx,dx,bx,sp,bp,
 * si,di,ds,es (10 GPRs) PLUS two extra `push ax` of local-var space, then
 * `mov bp,sp`. So BP+0..3 is locals, BP+4 is ES, ..., BP+22 is AX.
 * The compiler treats `r` as living at BP+0, so we pad 4 bytes at the
 * front of the struct to align fields with their actual stack positions.
 */
typedef struct {
    unsigned _pad_a, _pad_b;   /* locals slot from prologue */
    unsigned es, ds, di, si, bp, sp, bx, dx, cx, ax;
    unsigned ip, cs, flags;
} INT_REGS;

void __interrupt __far __loadds my_int2fh(INT_REGS r)
{
    if ((r.ax >> 8) == MUX_ID) {
        switch (r.ax & 0xFF) {
        case MUX_CHECK:
            r.ax = (r.ax & 0xFF00) | MUX_INSTALLED;
            return;
        case MUX_GET_PSP:
            r.ax = (r.ax & 0xFF00) | MUX_INSTALLED;
            r.es = _psp;
            return;
        case MUX_UNINSTALL:
            _dos_setvect(0x09, old_int9h);
            _dos_setvect(0x28, old_int28h);
            _dos_setvect(0x2F, old_int2fh);
            r.ax = (r.ax & 0xFF00) | MUX_INSTALLED;
            return;
        }
    }

    _chain_intr(old_int2fh);
}

/* ============================================================
   TRANSIENT CODE (kept too -- not trimmed in v1)
   ============================================================ */

#define FUJI_SIGNATURE  "FUJI"

typedef struct {
    unsigned char query;
    char          signature[4];
    unsigned char unit;
} fuji_ioctl_query;

unsigned char find_first_fujinet_drive(void)
{
    int drive;
    union REGS r;
    struct SREGS sr;
    fuji_ioctl_query query;

    for (drive = 3; drive <= 26; drive++) {
        memset(&query, 0, sizeof(query));
        r.h.ah = 0x44;
        r.h.al = 0x04;
        r.h.bl = (unsigned char) drive;
        r.x.cx = sizeof(query);
        r.x.dx = FP_OFF(&query);
        sr.ds  = FP_SEG(&query);
        int86x(0x21, &r, &r, &sr);

        if (!(r.x.cflag & INTR_CF) &&
            memcmp(query.signature, FUJI_SIGNATURE, 4) == 0 &&
            query.unit == 0)
            return 'A' + drive - 1;
    }
    return 0;
}

int probe_installed(unsigned *out_psp)
{
    union REGS r;
    struct SREGS sr;

    r.x.ax = ((unsigned)MUX_ID << 8) | MUX_CHECK;
    int86x(0x2F, &r, &r, &sr);
    if (r.h.al != MUX_INSTALLED)
        return 0;

    if (out_psp) {
        r.x.ax = ((unsigned)MUX_ID << 8) | MUX_GET_PSP;
        sr.es = 0;
        int86x(0x2F, &r, &r, &sr);
        *out_psp = sr.es;
    }
    return 1;
}

extern unsigned end;   /* linker-supplied: end of DGROUP */

int do_install(void)
{
    unsigned tsr_paras;
    unsigned highest_seg;

    if (probe_installed(NULL)) {
        printf("CFGTSR already installed.\n");
        return 1;
    }

    first_drive = find_first_fujinet_drive();
    if (!first_drive) {
        printf("No FujiNet drive (slot 0) found. Is FUJINET.SYS loaded?\n");
        return 1;
    }

    old_int9h  = _dos_getvect(0x09);
    old_int28h = _dos_getvect(0x28);
    old_int2fh = _dos_getvect(0x2F);

    _dos_setvect(0x09, my_int9h);
    _dos_setvect(0x28, my_int28h);
    _dos_setvect(0x2F, my_int2fh);

    printf("CFGTSR installed.  Hotkey: Ctrl+Alt+M   Drive: %c:\n",
           first_drive);

    /* Paragraphs from PSP through end of DGROUP (code + data + stack).
       Same calculation fnshare uses -- DOS 31h also resizes our memory
       block down to this size, freeing the rest for COMMAND.COM to reload. */
    _asm mov highest_seg, ds;
    tsr_paras = highest_seg + (((unsigned)&end) / 16) + 1 - _psp;

    _dos_keep(0, tsr_paras);
    return 0;  /* not reached */
}

int do_uninstall(void)
{
    unsigned tsr_psp = 0;
    union REGS r;
    struct SREGS sr;
    unsigned far *env_p;

    if (!probe_installed(&tsr_psp)) {
        printf("CFGTSR not installed.\n");
        return 1;
    }

    r.x.ax = ((unsigned)MUX_ID << 8) | MUX_UNINSTALL;
    int86x(0x2F, &r, &r, &sr);
    if (r.h.al != MUX_INSTALLED) {
        printf("Uninstall request was not acknowledged.\n");
        return 1;
    }

    /* Free environment block, then the program block itself */
    env_p = MK_FP(tsr_psp, 0x002C);
    if (*env_p) {
        r.h.ah = 0x49;
        sr.es  = *env_p;
        int86x(0x21, &r, &r, &sr);
    }
    r.h.ah = 0x49;
    sr.es  = tsr_psp;
    int86x(0x21, &r, &r, &sr);

    printf("CFGTSR uninstalled.\n");
    return 0;
}

static void show_usage(void)
{
    printf("CFGTSR -- FujiNet autorun config helper\n");
    printf("Usage: CFGTSR /I | /U\n");
    printf("  /I   install TSR (hotkey Ctrl+Alt+M brings up the menu)\n");
    printf("  /U   uninstall TSR\n");
    printf("\n");
    printf("The TSR auto-detects which DOS drive letter FUJINET.SYS\n");
    printf("assigned to FujiNet slot 0 (the autorun.img drive) and uses\n");
    printf("that for the M)ount and R)un Config actions.  The detected\n");
    printf("drive letter is shown at install time.\n");
}

int main(int argc, char **argv)
{
    if (argc == 2 && (argv[1][0] == '/' || argv[1][0] == '-')) {
        char opt = (char) toupper(argv[1][1]);
        if (opt == 'I') return do_install();
        if (opt == 'U') return do_uninstall();
    }

    show_usage();
    return (argc == 1) ? 0 : 1;
}
