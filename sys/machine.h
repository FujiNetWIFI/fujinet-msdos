#ifndef _MACHINE_H
#define _MACHINE_H

/*
 * Per-machine constants for the C side of the driver.
 *
 * The makefile defines exactly one MACH_* symbol (none at all means the
 * IBM PC default), and it passes the same symbol to wasm, where portio.asm
 * uses it to pick a backend under mach/.  Keep the two in step.
 *
 * Every machine must define:
 *   MACH_NAME          what to call the machine in the banner
 *   MACH_CHIP          the serial controller, for the banner
 *   MACH_DEFAULT_BPS   default FUJI_BPS
 *   PORT_BAUD_BASE     baud generator input after the /16 divide, so that
 *                      the divisor is PORT_BAUD_BASE / bps
 *   MACH_PORT_BASE     default port base (I/O address, or register segment
 *                      on memory-mapped machines)
 *   MACH_CONSOLE       how to get a character on screen (see below)
 *   MACH_HAS_8250      1 if the 8250-family probe in id8250.c is meaningful
 */

/* Console output backends.  A driver may only use a mechanism that is safe
 * to call from inside DOS request handling, because commands.c and
 * dispatch.c print diagnostics at runtime, not just during INIT.
 *   CONSOLE_BIOS_TTY - INT 10h AH=0Eh.  PC only.
 *   CONSOLE_DOS_FAST - INT 29h, DOS fast console output.  Present on
 *                      MS-DOS 2.0+, reaches the CON driver without going
 *                      through the INT 21h dispatcher, so it is re-entrant
 *                      enough for a driver.  Direct video writes are not an
 *                      option on any of these machines: the Apricot and
 *                      Victor screen buffers hold indirect glyph
 *                      descriptors rather than character codes, and the
 *                      Sanyo has no character cells at all.
 */
#define CONSOLE_BIOS_TTY	0
#define CONSOLE_DOS_FAST	1

#if defined(MACH_VICTOR9K)

/* Victor 9000 / ACT Sirius 1.  8088 at 5 MHz.
 * uPD7201 MPSC channel A, memory mapped at E004:0000 (data) and E004:0002
 * (control/status) - this machine has no I/O address space at all.
 * Baud from 8253 counter 0 at E002:0000, 1.25 MHz in, MPSC in x16 mode.
 * Source: MAME src/mame/act/victor9k.cpp, cross-checked against the Victor
 * boot ROM's own channel A setup. */
#define MACH_NAME		"Victor 9000"
#define MACH_CHIP		"uPD7201 MPSC"
#define MACH_DEFAULT_BPS	9600UL
#define PORT_BAUD_BASE		78125UL
#define MACH_PORT_BASE		0xE004
#define MACH_CONSOLE		CONSOLE_DOS_FAST
#define MACH_HAS_8250		0

#elif defined(MACH_APRICOT)

/* ACT Apricot PC / Xi.  8086 at 5 MHz.
 * Z80 SIO channel A at I/O 0x60 (data) and 0x62 (control/status).
 * Baud from 8253 counter 2 at I/O 0x58, 2 MHz in, SIO in x16 mode, routed
 * through a 74LS153 mux.  Channel B is the keyboard - do not disturb it.
 * Source: MAME src/mame/act/apricot.cpp. */
#define MACH_NAME		"Apricot PC/Xi"
#define MACH_CHIP		"Z80 SIO ch A"
#define MACH_DEFAULT_BPS	9600UL
#define PORT_BAUD_BASE		125000UL
#define MACH_PORT_BASE		0x60
#define MACH_CONSOLE		CONSOLE_DOS_FAST
#define MACH_HAS_8250		0

#elif defined(MACH_APRICOTF)

/* ACT Apricot F1 / F2 / F10.  8086.
 * Z80 SIO channel B at I/O 0x24 (data) and 0x26 (control/status) - note
 * this is the opposite channel from the Apricot PC.
 * Baud from Z80 CTC channel 1 at I/O 0x10, 153.846 kHz in, x16, so 9600 is
 * the ceiling.
 * Source: MAME src/mame/act/apricotf.cpp. */
#define MACH_NAME		"Apricot F1"
#define MACH_CHIP		"Z80 SIO ch B"
#define MACH_DEFAULT_BPS	9600UL
#define PORT_BAUD_BASE		9615UL
#define MACH_PORT_BASE		0x24
#define MACH_CONSOLE		CONSOLE_DOS_FAST
#define MACH_HAS_8250		0

#elif defined(MACH_SANYO550)

/* Sanyo MBC-550 / MBC-555.  8088 at 3.58 MHz.
 * i8251A at I/O 0x28 (data) and 0x2A (status/control).  The machine's I/O
 * decode shifts addresses left by one, which is why these are not the 0x14
 * and 0x15 that the chip select would suggest.  A second 8251 at 0x38/0x3A
 * is the keyboard - do not touch it.
 * Baud from 8253 counter 2 at I/O 0x20, 1.79 MHz in, x16.  9600 carries
 * -2.9% clock error; drop to FUJI_BPS=4800 if it proves marginal.
 * Source: MAME src/mame/sanyo/mbc55x.cpp. */
#define MACH_NAME		"Sanyo MBC-55x"
#define MACH_CHIP		"i8251A"
#define MACH_DEFAULT_BPS	9600UL
#define PORT_BAUD_BASE		111860UL
#define MACH_PORT_BASE		0x28
#define MACH_CONSOLE		CONSOLE_DOS_FAST
#define MACH_HAS_8250		0

#elif defined(MACH_TANDY2K)

/* Tandy 2000.  80186 at 8 MHz.
 * i8251A at I/O 0x10 (data) and 0x12 (status/control).
 * Baud from 8253 counter 1 at I/O 0x40, 2 MHz in, x16, gated by an
 * external-clock select that must be left on internal.
 * Source: MAME src/mame/trs/tandy2k.cpp. */
#define MACH_NAME		"Tandy 2000"
#define MACH_CHIP		"i8251A"
#define MACH_DEFAULT_BPS	9600UL
#define PORT_BAUD_BASE		125000UL
#define MACH_PORT_BASE		0x10
#define MACH_CONSOLE		CONSOLE_DOS_FAST
#define MACH_HAS_8250		0

#else

/* IBM PC and compatibles - the default.
 * 8250/16450/16550 at the usual COM port addresses, baud divisor derived
 * from the 1.8432 MHz UART crystal. */
#define MACH_PC			1
#define MACH_NAME		"IBM PC"
#define MACH_CHIP		"8250"
#define MACH_DEFAULT_BPS	115200UL
#define PORT_BAUD_BASE		115200UL
#define MACH_PORT_BASE		0x3f8
#define MACH_CONSOLE		CONSOLE_BIOS_TTY
#define MACH_HAS_8250		1

#endif

/*
 * Baud divisor.  The PC's base is an exact multiple of every rate it
 * supports, so plain division is exact there and is kept as-is to leave
 * the PC's generated code alone.  The other machines' baud bases are not
 * exact multiples, and truncating can be much worse than rounding - on the
 * Sanyo, 9600 truncates to divisor 11 (+5.9% error, unusable) where it
 * rounds to 12 (-2.9%, works).
 */
#ifdef MACH_PC
#define PORT_BAUD_DIVISOR(bps)	(PORT_BAUD_BASE / (bps))
#else
#define PORT_BAUD_DIVISOR(bps)	((PORT_BAUD_BASE + (bps) / 2) / (bps))
#endif

#endif /* _MACHINE_H */
