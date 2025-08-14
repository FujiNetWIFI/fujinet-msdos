/**
 * @brief   FCONFIG - Display FujiNet Config
 * @author  Thomas Cherryhomes
 * @email   thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE.md for details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <i86.h>
#include <dos.h>

struct _AdapterConfig
{
    char ssid[33];
    char hostname[64];
    unsigned char localIP[4];
    unsigned char gateway[4];
    unsigned char netmask[4];
    unsigned char dnsIP[4];
    unsigned char macAddress[6];
    unsigned char bssid[6];
    char fn_version[15];
} ac;

union REGS r;
struct SREGS sr;

void fetch_adapter_config(void)
{
    r.h.dl = 0x40;     // Read
    r.h.ah = 0xe8;     // Get adapter config
    r.h.al = 0x70;     // FUJI device
    r.x.cx = 0x0000;   // Not used
    r.x.si = 0x0000;   // Not used
    r.x.di = sizeof(struct _AdapterConfig); // Size of payload
    sr.es  = FP_SEG(&ac); // Destination buffer Segment
    r.x.bx = FP_OFF(&ac); // Destination buffer offset
    int86x(0xF5,&r,&r,&sr);

    if (r.h.al != 'C')
    {
        printf("ERROR: Could not read adapter configuration.\n");
        exit(1);
    }
}

void show_adapter_config(void)
{
    printf("\nCurrent FUJINET Network Config\n\n");
    printf("%8s:%30s\n","SSID",ac.ssid);
    printf("%8s:%30s\n","Hostname",ac.hostname);
    printf("%8s:%3u.%3u.%3u.%3u\n","Local IP",ac.localIP[0],ac.localIP[1],ac.localIP[2],ac.localIP[3]);
    printf("%8s:%3u.%3u.%3u.%3u\n","Netmask",ac.netmask[0],ac.netmask[1],ac.netmask[2],ac.netmask[3]);
    printf("%8s:%3u.%3u.%3u.%3u\n","Gateway",ac.gateway[0],ac.gateway[1],ac.gateway[2],ac.gateway[3]);
    printf("%8s:%3u.%3u.%3u.%3u\n","DNS",ac.dnsIP[0],ac.dnsIP[1],ac.dnsIP[2],ac.dnsIP[3]);
    printf("%8s:%02X:%02X:%02X:%02X:%02X:%02X\n","MAC",ac.macAddress[0],ac.macAddress[1],ac.macAddress[2],ac.macAddress[3],ac.macAddress[4],ac.macAddress[5]);
    printf("%8s:%02X:%02X:%02X:%02X:%02X:%02X\n","BSSID",ac.bssid[0],ac.bssid[1],ac.bssid[2],ac.bssid[3],ac.bssid[4],ac.bssid[5]);    
}

void main(void)
{
    fetch_adapter_config();
    show_adapter_config();
}
