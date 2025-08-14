/**
 * @brief   Scan and set WiFi Network Name (SSID)
 * @author  Thomas Cherryhomes
 * @email   thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <dos.h>
#include <conio.h>

#define INT_F5 0xF5    // FUJINET.SYS provides INT F5 for Network Services

typedef struct _network_config
{
  char ssid[33];
  char password[64];
} NetworkConfig;

typedef struct _scan_result
{
  char ssid[33];        // Entry SSID
  unsigned char rssi;   // Entry RSSI (strength)
} Result;

/**
 * @brief simple input for get password, terminate on ENTER
 * @param s Pointer to target string
 */
void get_password(char *s)
{
  int i=0;
  
  while(1)
    {
      int c = getch();
      char o = '*';
      
      switch(c)
	{
	case 0x08:
	  if (i)
	    {
	      i--;
	      o = c;
	    }
	  break;
	case 0x0D:
	  return;
	default:
	  s[i] = c;
	}
      
      putchar(o);
    }
    
}

/**
 * @brief return signal strength bars given rssi
 * @param rssi The signal strength
 * @return const char string containing graph [***]
 */
const char *print_rssi(signed char rssi)
{
  if (rssi > -50)
    return "***";
  else if (rssi > -70)
    return "**";
  else
    return "*";
}

/**
 * @brief Scan and list available SSIDs
 * @return 0 if successful, 1 if not. Also returns selected SSID.
 */
int scan(unsigned char *nn)
{
  union REGS r;
  struct SREGS sr;
  
  // Scan and return # of networks //////////////////////////////
  r.h.dl = 0x40;        // Read
  r.h.al = 0x70;        // FujiNet device
  r.h.ah = 0xFD;        // Scan Networks
  r.x.cx = 0x0000;      // No aux1
  r.x.si = 0x0000;      // No aux2
  sr.es = FP_SEG(nn);  // destination segment 
  r.x.bx = FP_OFF(nn); // destination offset
  r.x.di = 0x0001;      // 1 byte payload.

  // Do the call
  int86x(INT_F5,&r,&r,&sr);

  if (r.h.al != 'C')
    printf("Could not scan networks. Aborting.");

  return r.h.al != 'C';
}

/**
 * @brief Return scan result by index i
 * @param i Scan result index to return (0 to nn-1)
 * @param ssid pointer to ssid string.
 * @return 0 if successful, 1 if not.
 */
int get_scan_result(unsigned char i, Result *result)
{
  union REGS r;
  struct SREGS sr;
  
  r.h.dl = 0x40;        // read
  r.h.al = 0x70;        // FujiNet Device
  r.h.ah = 0xFC;        // Get SSID Result
  r.h.cl = i;           // scan result index
  r.h.ch = 0x00;
  r.x.si = 0x0000;
  sr.es = FP_SEG(result);
  r.x.bx = FP_OFF(result);
  r.x.di = sizeof(result); // Payload length
  
  // Do the call.
  int86x(INT_F5,&r,&r,&sr);

  return r.h.al != 'C';
}

/**
 * @brief Display scan results
 * @param nn Number of networks returned from scan()
 * @return 0 if successful, 1 if not.
 */
int show_scan_results(unsigned char nn)
{
  int i=0;
  Result result;
  
  if (!nn)
    {
      printf("No Networks Found. Exiting.\n");
      return 0;
    }
  
  for (i=0;i<nn;i++)
    {
      char ssid[33];

      memset(ssid,0,sizeof(ssid));
      
      if (get_scan_result(i,&result))
	{
	  printf("Unable to read next entry. Exiting.\n");
	  return 1;
	}
      else
	{
	  printf("%03u: %33s [%s]\n",
		 i,
		 result.ssid,
		 print_rssi(result.rssi));
	}
    }

  printf("\n");

  return 0;
}

/**
 * @brief select SSID from scanned list, O for other, or ENTER to exit.
 * @param 
 */
int select_ssid(NetworkConfig *cfg)
{
  char input_str[80];
  unsigned char selected_ssid;

  while (1)
    {
      memset(input_str,0,sizeof(input_str));

      printf("Enter SSID #, or \n 'O' for other SSID, or \n [Enter] to abort. >>");

      gets(input_str);

      if (input_str[0] == toupper('O'))
	{
	  memset(input_str,0,sizeof(input_str)); 
	  printf("Enter SSID name: ");
	  gets(input_str);
	  if (!input_str[0])
	    return 1;
	  else
	    {
	      strncpy(cfg->ssid,input_str,32);
	      return 0;
	    }
	}
      else if (!input_str[0])
	return 1;
      else
	{
	  selected_ssid = atoi(input_str);
	  
	}
    }
}

/**
 * @brief Set SSID using INT F5 call, using values in cfg.
 * @return 0 if successful, 1 if not.
 */
int set_ssid(NetworkConfig *cfg)
{
  union REGS r;
  struct SREGS sr;
  
  r.h.dl = 0x80;        // Write
  r.h.al = 0x70;        // FujiNet Device
  r.h.ah = 0xFB;        // Set SSID
  r.h.cl = 0x01;        // set Save
  r.h.ch = 0x00;
  r.x.si = 0x0000;
  sr.es = FP_SEG(cfg);
  r.x.bx = FP_OFF(cfg);
  r.x.di = sizeof(cfg); // Payload length

  // Do the call.
  int86x(INT_F5,&r,&r,&sr);
  
  return r.h.al != 'C';
}

/**
 * @brief Show usage
 */
int usage(char *argv[])
{
  printf("%s [desired_ssid] [password] [/S]\n\n",argv[0]);
  printf("\t [desired_ssid] \t The SSID to use (32 chars max)\n");
  printf("\t [password] \t The password to use (64 chars max)\n");
  printf("\t /S \t Perform scan and display only.\n\n");
  printf("If no parameters are given, they will be input interactively.\n\n");

  return 0;
}

/**
 * @brief get arguments, pass to set_ssid
 */
int main(int argc, char *argv[])
{
  NetworkConfig cfg;
  unsigned char nn=0;

  memset(&cfg, 0, sizeof(NetworkConfig));
  
  if (argc>2 && argv[1][0] == '/' && argv[1][1] == toupper('S'))
    {
      if (scan(&nn))
	return 1;
      if (show_scan_results(nn))
	return 1;
    }

  if (argc>2 && argv[1][0] == '/' && argv[1][1] == '?')
    return usage(argv);

  if (argc>2)
    strncpy(&cfg.password,argv[2],sizeof(&cfg.password));

  if (argc>1)
    strncpy(&cfg.ssid,    argv[1],sizeof(&cfg.ssid));

  if (!cfg.ssid[0])
    {
      if (scan(&nn))
	return 1;

      if (show_scan_results(nn))
	return 1;

      if (select_ssid(&cfg))
	return 1;

      // We didn't select an SSID. Simply exit.
      if (!cfg.ssid[0])
	return 0;
    }

  if (!cfg.password[0])
      get_password(&cfg.password);

  return set_ssid(&cfg);
}
