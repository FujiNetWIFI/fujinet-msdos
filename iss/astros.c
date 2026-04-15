/**
 * Display astronauts currently in space
 */

#include <dos.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <conio.h>
#include "grlib.h"
#include "map.h"

static const char astros_url[256] = "N:HTTP://api.open-notify.org/astros.json";

static union REGS ar;
static struct SREGS asr;

static struct {
    unsigned short bw;
    unsigned char connected;
    unsigned char error;
} as;

static void astro_query(const char *path, char *result, unsigned int result_size)
{
    unsigned int len = strlen(path) + 1;

    ar.h.dl = 0x80;
    ar.h.ah = 'Q';
    ar.h.al = 0x71;
    ar.x.cx = 0x0000;
    ar.x.si = 0x0000;
    ar.h.dh = 0;
    asr.es = FP_SEG(path);
    ar.x.bx = FP_OFF(path);
    ar.x.di = len;
    int86x(0xF5, &ar, &ar, &asr);

    ar.h.dl = 0x40;
    ar.h.ah = 'S';
    ar.h.al = 0x71;
    ar.x.cx = 0x0000;
    ar.x.si = 0x0000;
    ar.h.dh = 0;
    asr.es = FP_SEG(&as);
    ar.x.bx = FP_OFF(&as);
    ar.x.di = sizeof(as);
    int86x(0xF5, &ar, &ar, &asr);

    if (as.bw >= result_size)
        as.bw = result_size - 1;

    ar.h.dl = 0x40;
    ar.h.ah = 'R';
    ar.h.al = 0x71;
    ar.x.cx = as.bw;
    ar.x.si = 0x0000;
    ar.h.dh = 5;
    asr.es = FP_SEG(result);
    ar.x.bx = FP_OFF(result);
    ar.x.di = as.bw;
    int86x(0xF5, &ar, &ar, &asr);

    result[as.bw] = '\0';
}

static void center_text(int y, const char *text)
{
    int x = (40 - (int)strlen(text)) / 2;
    if (x < 0) x = 0;
    gr_text(x, y, (char *)text);
}

void astros(void)
{
    char num_s[8];
    char name[64];
    char craft[32];
    char msg[128];
    char tmp[64];
    char hdr[41];
    int n, i, key;

    center_text(12, "Fetching crew data...");

    ar.h.dl = 0x80;
    ar.h.ah = 'O';
    ar.h.al = 0x71;
    ar.h.cl = 0x0c;
    ar.h.ch = 0x00;
    ar.x.si = 0x0000;
    ar.h.dh = 2;
    asr.es = FP_SEG(astros_url);
    ar.x.bx = FP_OFF(astros_url);
    ar.x.di = 256;
    int86x(0xF5, &ar, &ar, &asr);

    ar.h.dl = 0x00;
    ar.h.ah = 0xFC;
    ar.h.al = 0x71;
    ar.h.cl = 0x00;
    ar.h.ch = 0x01;
    ar.x.si = 0x0000;
    ar.h.dh = 2;
    int86(0xF5, &ar, &ar);

    ar.h.dl = 0x00;
    ar.h.ah = 'P';
    ar.h.al = 0x71;
    ar.x.cx = 0x0000;
    ar.x.si = 0x0000;
    ar.h.dh = 0;
    int86(0xF5, &ar, &ar);

    memset(num_s, 0, sizeof(num_s));
    astro_query("/number", num_s, sizeof(num_s));
    n = atoi(num_s);
    if (n <= 0) {
        map();
        return;
    }

    sprintf(hdr, "There are %d people in space!", n);
    map();
    center_text(0, hdr);

    for (i = 0; i < n; i++) {
        memset(name, 0, sizeof(name));
        memset(craft, 0, sizeof(craft));

        sprintf(tmp, "/people/%d/name", i);
        astro_query(tmp, name, sizeof(name));

        sprintf(tmp, "/people/%d/craft", i);
        astro_query(tmp, craft, sizeof(craft));

        sprintf(msg, "%s is on %s", name, craft);
        center_text(12, msg);

        if (i < n - 1)
            center_text(22, "Press a key for more  [ESC]=stop");
        else
            center_text(22, "Press a key");

        key = getch();
        if (key == 0x1B)
            break;

        if (i < n - 1) {
            map();
            center_text(0, hdr);
        }
    }

    map();
}
