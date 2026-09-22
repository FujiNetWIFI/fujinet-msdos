#ifndef _PRINT_H
#define _PRINT_H

#include <stdint.h>
#include <stdarg.h>
#include "machine.h"

#undef DOS_SAFE
#ifdef DOS_SAFE
/* Print a single character with DOS */
extern void printChar(char);
#pragma aux printChar =		\
  "mov ah, 0x2"			\
  "int 0x21"			\
  __parm [__dl]			\
  __modify [__ax __di __es];

/* Use INT 21h to print. Buffer MUST be terminated with '$' so BIOS
   knows when to stop */
extern void printDTerm(const char *);
#pragma aux printDTerm =      \
    "mov    ah, 0x9"          \
    "int    0x21"             \
    __parm [__dx]             \
    __modify [__ax __di __es];
#else /* !DOS_SAFE */

#if MACH_CONSOLE == CONSOLE_BIOS_TTY
/* Print a single character with BIOS */
extern void printChar(char);
#pragma aux printChar =         \
  "mov ah, 0xE"                 \
  "int 0x10"                    \
  __parm [__al]                 \
  __modify [__ax __cx];

#define print_probe()		/* nothing to choose */
#define print_resident()	/* INT 10h is safe at runtime */

#else /* MACH_CONSOLE == CONSOLE_DOS_FAST */

/* Machines with no INT 10h print through DOS instead.  printChar has to be
 * a real function here because the mechanism is chosen at INIT: see
 * print_probe() below and the discussion in machine.h. */
extern void printChar(char);

#define PRINT_MODE_FAST	0	/* INT 29h - safe at INIT and at runtime */
#define PRINT_MODE_DOS	1	/* INT 21h AH=02h - only safe during INIT */
#define PRINT_MODE_OFF	2	/* no safe mechanism; stay quiet */

extern uint8_t print_mode;

/* Pick a console mechanism.  Call once, early in INIT, before printing. */
extern void print_probe(void);

/* Call at the end of INIT.  Retires any mechanism that is not safe to use
 * from inside DOS request handling, so the runtime diagnostics in
 * commands.c and dispatch.c can never corrupt DOS. */
extern void print_resident(void);

#endif /* MACH_CONSOLE */

#endif /* DOS_SAFE */

extern void printHex(uint16_t val, uint16_t width, char leading);
extern void printHex32(uint32_t val, uint16_t width, char leading);
extern void printDec(uint16_t val, uint16_t width, char leading);
extern void dumpHex(void far *ptr, uint16_t count, uint16_t address);
extern void printString(const char *str);
extern void printFarString(const char far *str);

extern void vconsolef(const char *format, va_list args);
extern void consolef(const char *format, ...);

#endif /* _PRINT_H */
