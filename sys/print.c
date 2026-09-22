#include "print.h"
#include <stdarg.h>
#include <ctype.h>

#if MACH_CONSOLE == CONSOLE_DOS_FAST
#include <dos.h>

/*
 * Console output for machines with no IBM-compatible INT 10h.
 *
 * INT 29h is DOS fast console output: it goes straight to the CON driver's
 * character routine without passing through the INT 21h dispatcher, so
 * unlike INT 21h AH=02h it is safe to call from inside DOS request
 * handling.  That matters because commands.c and dispatch.c print
 * diagnostics at runtime, not only during INIT.
 *
 * It is a documented MS-DOS 2.0+ facility, but these are OEM ports, so we
 * probe for it rather than assume it.  If it is missing we still get the
 * banner out through DOS during INIT and then go quiet, which costs
 * diagnostics but can never corrupt DOS.
 */

uint8_t print_mode = PRINT_MODE_DOS;

static void putcFast(char c);
#pragma aux putcFast =		\
  "int 0x29"			\
  __parm [__al]			\
  __modify [__ax __bx __cx __dx __si __di];

static void putcDos(char c);
#pragma aux putcDos =		\
  "mov ah, 0x2"			\
  "int 0x21"			\
  __parm [__dl]			\
  __modify [__ax __di __es];

void printChar(char c)
{
  switch (print_mode) {
  case PRINT_MODE_FAST:
    putcFast(c);
    break;

  case PRINT_MODE_DOS:
    putcDos(c);
    break;

  default:
    /* PRINT_MODE_OFF - no mechanism that is safe here */
    break;
  }

  return;
}

void print_probe(void)
{
  void far *vec = *(void far * far *) MK_FP(0, 0x29 * 4);

  /* A null vector means this DOS has no fast console output. */
  print_mode = vec ? PRINT_MODE_FAST : PRINT_MODE_DOS;

  return;
}

void print_resident(void)
{
  /* INT 21h is not re-entrant, so it cannot survive into the resident
     driver.  INT 29h can. */
  if (print_mode == PRINT_MODE_DOS)
    print_mode = PRINT_MODE_OFF;

  return;
}
#endif /* MACH_CONSOLE == CONSOLE_DOS_FAST */

void printHex(uint16_t val, uint16_t width, char leading)
{
  uint16_t digits, tval;
  char c;


  for (tval = val, digits = 0; tval; tval >>= 4, digits++)
    ;
  if (!digits)
    digits = 1;

  for (; digits < width; width--)
    printChar(leading);

  while (digits) {
    digits--;
    c = (val >> 4 * digits) & 0xf;
    printChar('0' + c + (c > 9 ? 7 : 0));
  }

  return;
}

void printHex32(uint32_t val, uint16_t width, char leading)
{
  uint16_t digits;
  uint32_t tval;
  char c;


  for (tval = val, digits = 0; tval; tval >>= 4, digits++)
    ;
  if (!digits)
    digits = 1;

  for (; digits < width; width--)
    printChar(leading);

  while (digits) {
    digits--;
    c = (val >> 4 * digits) & 0xf;
    printChar('0' + c + (c > 9 ? 7 : 0));
  }

  return;
}

void printDec(uint16_t val, uint16_t width, char leading)
{
  uint16_t digits, tval, tens;


  for (tval = val, digits = 0; tval; tval /= 10, digits++)
    ;
  if (!digits)
    digits = 1;
  for (tval = digits - 1, tens = 1; tval; tval--, tens *= 10)
    ;

  for (; digits < width; width--)
    printChar(leading);

  while (digits) {
    digits--;
    printChar('0' + (val / tens) % 10);
    tens /= 10;
  }

  return;
}

void printDec32(uint32_t val, uint16_t width, char leading)
{
  uint32_t tval, tens;
  uint16_t digits;


  for (tval = val, digits = 0; tval; tval /= 10, digits++)
    ;
  if (!digits)
    digits = 1;
  for (tval = digits - 1, tens = 1; tval; tval--, tens *= 10)
    ;

  for (; digits < width; width--)
    printChar(leading);

  while (digits) {
    digits--;
    printChar('0' + (val / tens) % 10);
    tens /= 10;
  }

  return;
}

void printString(const char *str)
{
  for (; str && *str; str++)
    printChar(*str);
  return;
}

void printFarString(const char far *str)
{
  for (; str && *str; str++)
    printChar(*str);
  return;
}

void dumpHex(void far *ptr, uint16_t count, uint16_t address)
{
  int outer, inner;
  uint8_t c, is_err;
  uint8_t far *buffer = (uint8_t far *) ptr;


  for (outer = 0; outer < count; outer += 16) {
    printHex(outer + address, 4, '0');
#ifdef DOS_SAFE
    printDTerm("  $");
#else
    printChar(' ');
    printChar(' ');
#endif /* DOS_SAFE */
    for (inner = 0; inner < 16; inner++) {
      if (inner + outer < count) {
        c = buffer[inner + outer];
	printHex(c, 2, '0');
	printChar(' ');
      }
      else {
#ifdef DOS_SAFE
	printDTerm("   $");
#else
	printChar(' ');
	printChar(' ');
	printChar(' ');
#endif /* DOS_SAFE */
      }
    }
#ifdef DOS_SAFE
    printDTerm(" |$");
#else
    printChar(' ');
    printChar('|');
#endif /* DOS_SAFE */
    for (inner = 0; inner < 16 && inner + outer < count; inner++) {
      c = buffer[inner + outer];
      if (c >= ' ' && c <= 0x7f)
	printChar(c);
      else
	printChar('.');
    }
#ifdef DOS_SAFE
    printDTerm("|\r\n$");
#else
    printChar('|');
    printChar('\r');
    printChar('\n');
#endif /* DOS_SAFE */
  }

  return;
}

void vconsolef(const char *format, va_list args)
{
  const char *pf;
  char leader;
  uint8_t width;


  for (pf = format; pf && *pf; pf++) {
    switch (*pf) {
    case '\n':
#ifdef DOS_SAFE
      printDTerm("\r\n$");
#else
      printChar('\r');
      printChar('\n');
#endif /* DOS_SAFE */
      break;

    case '%':
      pf++;
      if (!*pf)
	break;

      if (*pf == 'c')
	printChar(va_arg(args, char));
      else {
	leader = ' ';
	width = 0;

	if (isdigit(*pf)) {
	  if (*pf == '0') {
	    leader = '0';
	    pf++;
	  }

	  for (width = 0; isdigit(*pf); pf++) {
	    width *= 10;
	    width += *pf - '0';
	  }
	}

	if (*pf == 'l') {
	  pf++;
	  if (*pf == 'x')
	    printHex32(va_arg(args, uint32_t), width, leader);
	  else if (*pf == 'i' || *pf == 'd')
	    printDec32(va_arg(args, uint32_t), width, leader);
	  else if (*pf == 's')
	    printFarString(va_arg(args, char far *));
	}
	else {
	  if (*pf == 'x')
	    printHex(va_arg(args, uint16_t), width, leader);
	  else if (*pf == 'i' || *pf == 'd')
	    printDec(va_arg(args, uint16_t), width, leader);
	  else if (*pf == 's')
	    printString(va_arg(args, char *));
	}
      }
      break;

    default:
      printChar(*pf);
      break;
    }
  }

  return;
}

void consolef(const char *format, ...)
{
  va_list args;


  va_start(args, format);
  vconsolef(format, args);
  va_end(args);
}
