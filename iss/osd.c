/**
 * OSD
 */

#include <stdio.h>
#include <time.h>
#include "grlib.h"
#include "ftime.h"

void osd(char *lat, char *lon, unsigned long t)
{
    time_t ti;
	char tmp[41];
        char buf[26];
        struct tm tod;
        
	gr_text(10,21,"CURRENT ISS POSITION");

        time(&ti);
        _localtime(&ti,&tod);
        
        sprintf(tmp,"          %s",_asctime(&tod,buf));
	gr_text(0,22,tmp);
	sprintf(tmp,"  LAT: %10s    LON: %10s",lat,lon);
	gr_text(0,23,tmp);
	gr_text(4,24,"[P]=Who is in space?  [ESC]=Quit");
}
