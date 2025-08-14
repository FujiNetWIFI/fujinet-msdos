/**
 * @brief   netcat - Terminal prototype
 * @author  Thomas Cherryhomes
 * @email   thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE for details
 */

#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <i86.h>

char url[256];
char buf[8192];
char txbuf[256];
char username[256];
char password[256];

union REGS r;
struct SREGS sr;

struct _status
{
	unsigned short bw;
	unsigned char connected;
	unsigned char err;
} status;

void nc_status(void)
{
    r.h.al = 0x00;

    r.h.dl = 0x40;
    r.h.ah = 'S';
    r.h.al = 0x71;
    r.x.cx = 0x0000;
    r.x.si = 0x0000;
    sr.es = FP_SEG(&status);
    r.x.bx = FP_OFF(&status);
    r.x.di = sizeof(status);
    int86x(0xF5,&r,&r,&sr);
    //delay(250);
}

int nc(char *s)
{
    int err = 0;

    strcpy(url,s);

    r.h.al = 0;

    delay(250);
    r.h.dl = 0x80;
    r.h.ah = 'O';
    r.h.al = 0x71;
    r.h.cl = 0x0c;
    r.h.ch = 0x00;
    r.x.si = 0x0000;
    sr.es = FP_SEG(url);
    r.x.bx = FP_OFF(url);
    r.x.di = sizeof(url);
    int86x(0xF5,&r,&r,&sr);
    delay(250);

    err = r.h.al;

    printf("Open Returned: 0x%02x",err);

    if (r.h.al != 'C')
    {
        /* Get error from status */
        nc_status();
        printf("\nCould not open URL, error: %u\n",status.err);
        goto bye;
    }

	/* connected, get initial status */

    nc_status();

    while(status.connected)
    {
        int i=0;
        int bw=0;

        //delay(1);

        while (kbhit())
        {
            txbuf[i++]=getch();
        }

        if (i)
        {
            r.h.al = 0;

            r.h.dl = 0x80;
            r.h.ah = 'W';
            r.h.al = 0x71;
            r.x.cx = 0x0001;
            r.x.si = 0x0000;
            sr.es = FP_SEG(txbuf);
            r.x.bx = FP_OFF(txbuf);
            r.x.di = i;
            int86x(0xF5,&r,&r,&sr);
            delay(250);
            i=0;
        }

        nc_status();

        if (!status.bw)
            continue;

        bw = (status.bw > sizeof(buf) ? sizeof(buf) : status.bw);

        r.h.al = 0;

        r.h.dl = 0x40;
        r.h.ah = 'R';
        r.h.al = 0x71;
        r.x.cx = bw;
        r.x.si = 0x0000;
        sr.es = FP_SEG(buf);
        r.x.bx = FP_OFF(buf);
        r.x.di = bw;
        int86x(0xF5,&r,&r,&sr);

        for (i=0;i<bw;i++)
            putchar(buf[i]);
    }

bye:
    r.h.al = 0;

    /* Send close */
    r.h.dl = 0x00;
    r.h.ah = 'C';
    r.h.al = 0x71;
    r.x.cx = 0x0000;
    r.x.si = 0x0000;
    int86(0xF5,&r,&r);

    err = r.h.al;

    return err;
}

int usage(void)
{
	printf("\nnc <N:url>\n");
	return 1;
}

int main(int argc, char *argv[])
{
	if (argc < 2)
		return usage();

	return nc(argv[1]);
}
