#include "commands.h"
#include "fujinet.h"
#include "dispatch.h"
#include "../sys/print.h"
#include <fuji_f5.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <dos.h>

#ifndef VERSION
#define VERSION "0.8"
#endif

#include <stdio.h>

#if defined(__WATCOMC__)

#define CC_VERSION_MINOR	(__WATCOMC__ % 100)
#if __WATCOMC__ > 1100
#define CC_VERSION_MAJOR	(__WATCOMC__ / 100 - 11)
#define CC_VERSION_NAME		"Open Watcom"
#else /* __WATCOMC__ > 1100 */
#define CC_VERSION_MAJOR	(__WATCOMC__ / 100)
#define CC_VERSION_NAME		"Watcom"
#endif /* __WATCOMC__ > 1100 */

#elif defined(__TURBOC__)

#define CC_VERSION_MAJOR	(__TURBOC__ / 0x100)
#define CC_VERSION_MINOR	(__TURBOC__ % 0x100)
#define CC_VERSION_NAME		"Turbo C"

#endif /* __WATCOMC__ */

union REGS regs;
extern void *config_env, *driver_end;

extern void set17(void);

#pragma data_seg("_CODE")

uint16_t parse_config(const uint8_t far *config_sys);

uint16_t Init_cmd(SYSREQ far *req)
{
  uint8_t err;
  uint16_t unused;


  regs.h.ah = 0x30;
  intdos(&regs, &regs);
  consolef("\nFujiNet Printer driver " VERSION
	   " " CC_VERSION_NAME " %i.%i"
	   " on MS-DOS %i.%i\n",
	   CC_VERSION_MAJOR, CC_VERSION_MINOR,
	   regs.h.al, regs.h.ah);
  unused = parse_config(req->init.bpb_ptr);
  environ = (char **) &config_env;

  req->init.end_ptr = MK_FP(getCS(), (uint8_t *) &driver_end - unused);

  consolef("Installed\n");
  set17();
  consolef("INT 17 functions installed\n");

  return OP_COMPLETE;
}

/* Parse CONFIG.SYS command line, returns number of bytes remaining in config_env */
#define IS_CONFIG_EOL(c) (c == '\r' || c == '\n')
uint16_t parse_config(const uint8_t far *config_sys)
{
  int idx, count;
  const uint8_t far *cfg, far *bcfg;
  char **cfg_env = (char **) &config_env;
  char *buf, *buf_max;
  uint8_t eq_flag;


#ifdef CONFIG_SYS_DEBUG
  consolef("CONFIG.SYS: ");
  for (cfg = config_sys; cfg && !IS_CONFIG_EOL(*cfg); cfg++)
    printChar(*cfg);
  consolef("\n");
  dumpHex(config_sys, 64);
#endif /* CONFIG_SYS_DEBUG */
  *cfg_env = NULL;
  buf = (char *) &cfg_env[1];
  buf_max = (char *) cfg_env + ((uint16_t) &driver_end - (uint16_t) &config_env);

  // Driver filename is everything before the first space
  for (cfg = config_sys; cfg && *cfg > ' ' && !IS_CONFIG_EOL(*cfg); cfg++)
    ;

  if (*cfg && *cfg != ' ')
    goto done;

  cfg++;
  // Skip any extra spaces
  while (*cfg == ' ')
    cfg++;
  if (IS_CONFIG_EOL(*cfg))
    goto done;

  bcfg = cfg;

  // Count how many options we have
  count = 0;
  while (1) {
    // Find end of this config option
    for (; cfg && *cfg != ' ' && !IS_CONFIG_EOL(*cfg); cfg++)
      ;
    count++;
    if (*cfg != ' ')
      break;

    // Skip any trailing spaces
    while (*cfg == ' ')
      cfg++;
  }

  // Start strings after pointer table + NULL
  buf = ((char *) cfg_env) + sizeof(char *) * (count + 1);

  // Convert options to null terminated environment variables
  for (idx = 0, cfg = bcfg; idx < count && buf < buf_max; idx++) {
    cfg_env[idx] = buf;

    // Find end of this config option
    for (eq_flag = 0; cfg && *cfg != ' ' && !IS_CONFIG_EOL(*cfg); cfg++) {
      if (*cfg == '=')
	eq_flag = 1;
      *buf = *cfg;
      buf++;
      if (buf == buf_max)
	break;
    }
    if (buf == buf_max)
      break;
    
    if (!eq_flag) {
      *buf = '=';
      buf++;
      if (buf == buf_max)
	break;
    }
    *buf = 0;
    buf++;

    // Skip any trailing spaces
    while (*cfg == ' ')
      cfg++;
  }

  cfg_env[idx] = 0;

 done:
  return buf - (char *) &config_env;
}
