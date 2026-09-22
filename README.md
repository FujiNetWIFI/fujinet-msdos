# FujiNet Tools - RS-232 for MS-DOS

MS-DOS utilities and drivers for the [FujiNet](https://github.com/FujiNetWIFI/fujinet-firmware) RS-232 adapter — an ESP32-based device that provides network access, virtual disk drives, and printer emulation to vintage computers via serial port.

## Components

| Output         | Description                                              |
|----------------|----------------------------------------------------------|
| `fujinet.sys`  | Block device driver — load in `CONFIG.SYS`               |
| `fn*.sys`      | The same driver for non-PC MS-DOS machines ([below](#non-pc-ms-dos-machines)) |
| `fujiprn.sys`  | Printer driver (redirects INT 17h to FujiNet)            |
| `fujicoms.lib` | FUJICOM RS-232 communications library                    |
| `fmount.exe`   | Mount and unmount FujiNet disk images as DOS drives      |
| `ncopy.exe`    | Interactive network file copy utility                    |
| `nget.exe`     | Download a file from a network URL                       |
| `fnshare.exe`  | TSR that mounts any share FujiNet can talk to as a drive |
| `setssid.exe`  | Configure FujiNet WiFi SSID and password                 |
| `iss.exe`      | ISS position tracker — HTTP/JSON demo                    |

## Non-PC MS-DOS Machines

`fujinet.sys` assumes IBM PC hardware: an 8250-family UART, the PC's
1.8432 MHz baud crystal, the BIOS tick counter at `0040:006Ch`, and
`INT 10h` for console output. Plenty of 8086/8088/80186 machines ran
MS-DOS 2.x without being PC compatible, so those get their own build of the
same driver:

| Machine | Driver | Serial hardware | Default BPS | Max |
|---------|--------|-----------------|-------------|-----|
| IBM PC and compatibles | `fujinet.sys` | 8250/16450/16550 | 115200 | 115200 |
| Victor 9000 / ACT Sirius 1 | `fnvic9k.sys` | uPD7201 MPSC ch A | 9600 | 38400 |
| Apricot PC / Xi | `fnapric.sys` | Z80 SIO ch A | 9600 | 9600 |
| Apricot F1 / F2 / F10 | `fnaprf.sys` | Z80 SIO ch B | 9600 | 9600 |
| Sanyo MBC-550 / 555 | `fnsanyo.sys` | i8251A | 9600 | 19200 |
| Tandy 2000 | `fnt2000.sys` | i8251A | 9600 | 9600 |

Pick the driver for your machine and load it exactly like `fujinet.sys`:

```
DEVICE=FNSANYO.SYS
```

Notes:

- **MS-DOS 2.0 or later is required.** `DEVICE=` did not exist in 1.25, so
  machines shipped with MS-DOS 1.x need a 2.x upgrade first.
- **9600 is the default off the PC**, not 115200. These machines derive the
  baud clock from a timer chip rather than a dedicated UART crystal, and
  9600 is the only rate that divides cleanly on all of them. It is also the
  FujiNet RS232 firmware default, so both ends agree out of the box.
- **The Sanyo's 9600 carries -2.9% clock error** — within an 8N1 frame's
  budget, but the tightest of the set. If a link is unreliable, try
  `FUJI_BPS=4800` (+1.3%).
- **`FUJI_PORT`'s `1`-`4` COM numbering is PC-only.** Elsewhere the hex form
  sets the register address directly, e.g. `FUJI_PORT=0x20` to use the
  Apricot F1's channel A instead of channel B.
- Only the driver is ported. The `.EXE` utilities reach FujiNet through the
  driver's `INT F5h` API and work, but their own screen output still goes
  through `INT 10h`, so expect it to be missing or garbled.

Adding another machine means writing one file, `sys/mach/<name>.inc`,
against the contract documented at the top of `sys/mach/pc.inc`, then
adding it to the table in `sys/machines.mk` and the `IFDEF` chain in
`sys/portio.asm`.

## Runtime Setup

Add the drivers to `CONFIG.SYS` before using any utilities:

```
DEVICE=FUJINET.SYS
DEVICE=FUJIPRN.SYS
```

`fujinet.sys` must be loaded for `fmount`, `setssid`, and other utilities to function.

## FUJICOM Environment Variables

These control the RS-232 connection to the FujiNet adapter:

| Variable  | Default | Description                                                |
|-----------|---------|------------------------------------------------------------|
| FUJI_PORT | 1       | Serial port to use: 1–4, or hex I/O address (e.g. `0x3F8`) |
| FUJI_BPS  | 115200  | Bits per second (9600, 19200, 115200, etc.)                |

## Build Directions

### Prerequisites: Open Watcom

#### Install

```sh
wget https://github.com/open-watcom/open-watcom-v2/releases/download/Current-build/open-watcom-2_0-c-linux-x64

mkdir ~/openwatcom
cd ~/openwatcom
unzip ../Downloads/open-watcom-2_0-c-linux-x64
```

#### Setup Env

Add to your shell profile or run before building:

```sh
export WATCOM=~/openwatcom
export PATH=$WATCOM/binl64:$WATCOM/binl:$PATH
export EDPATH=$WATCOM/eddat
export INCLUDE=$WATCOM/h
```

### Make Targets

```sh
make          # build all components (PC)
make machines # build the non-PC machine drivers
make clean    # remove all build artifacts
make builds   # build and copy outputs to builds/
make zip      # build and package outputs into fn-msdos.zip
make disk     # build and write a 1.44MB floppy image (fn-msdos.img)
make disk USE_GIT_REF=1  # same, but names the image fn-<git-hash>.img
```

`make disk` requires [mtools](https://www.gnu.org/software/mtools/) (`mformat`, `mcopy`).

`make machines` builds the non-PC drivers listed
[above](#non-pc-ms-dos-machines); `make builds-machines` also copies them
into `builds/`. To build just one:

```sh
make -C sys MACHINE=victor9k    # or apricot apricotf sanyo550 tandy2k
```

## Further Reading

- [FUJICOM-Protocol.md](FUJICOM-Protocol.md) — RS-232 protocol specification (command frames, SLIP framing, pin assignments)
- [fujinet-bios.md](fujinet-bios.md) — INT F5 BIOS interface specification with C and assembly examples

## License

GPL v3 — see [license](license).
