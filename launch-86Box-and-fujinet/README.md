# launch-86box-and-fujinet

## SYNOPSIS

This is a script that I use on my Arch Linux box to launch FujiNet-PC and
86Box together, wired up over TCP.

**This used to work by allocating a host PTY and scraping its path into
`fnconfig.ini` via 86Box's serial passthrough.** 86Box 6.0 replaced that
mechanism with a new character-device API and the PTY dance stopped working.
It's gone now — 86Box talks to FujiNet directly over a loopback TCP socket
via a new `fujinet` character device (from
[FujiNetWIFI/86Box](https://github.com/FujiNetWIFI/86Box), branch
`feat/fujinet`), the same way FujiNet-PC's other BoIP-connected platforms
(CoCo, Adam) already do. No PTY, no `sed`, no `mode = 3`.

## PREREQUISITES

You need the following

* FujiNet Firmware (RS232 target): https://github.com/FujiNetWIFI/fujinet-firmware
* 86Box, built from https://github.com/FujiNetWIFI/86Box (`feat/fujinet` branch)
* 86Box-roms
* This repository's `sys/` folder for the CONFIG.SYS driver

## CONFIGURATION

### 86Box VM config (86box.cfg) — 86Box 6.0+ with the `feat/fujinet` fork

Select the `fujinet` character device on the COM port FujiNet should use:

```cfg
[Ports (COM & LPT)]
serial1_enabled = 1
serial1_device = fujinet

[FujiNet (BoIP) #1]
host = 127.0.0.1
port = 1987
```

`host`/`port` default to `127.0.0.1:1987` even if the section and keys are
omitted entirely.

__Note__: The PCjr machine in 86Box adds its own internal NS8250 serial at device
slot 0 (the "COM1" slot), and then blocks all other serial ports from
initialising. The PCjr hardware is wired at 0x2F8 (the standard COM2 address),
but from 86Box's perspective it lives on the COM1 char-device slot. The
practical result is:

- `86box.cfg` only needs `serial1_device = fujinet` — the COM2 entry does
  nothing on the PCjr.
- Inside DOS, the driver still uses `FUJI_PORT=2` because the hardware address
  really is 0x2F8.

Direction matters: **FujiNet listens, 86Box connects out** to it — the same
inversion openMSX's `FujiNet.cc` device uses. FujiNet-PC's RS232 target
already defaults its BoIP channel to listening mode, so nothing on the
FujiNet side needs to change beyond enabling BoIP (see below).

### fnconfig.ini

```ini
[BOIP]
enabled=1
host=127.0.0.1
port=1987
```

The script maintains this section for you on every run — see
`launch-86box-and-fujinet.sh` / `IBM_5150.sh`.

Also, the top of the launch script has a few important variables:

```sh
CONFIG_DIR="/home/thomc/Workspace/fujinet-pc-rs232/build/dist"
VM_DIR="/home/thomc/Vintage/IBM 4860 PCjr"
CONFIG_FILE="$CONFIG_DIR/fnconfig.ini"
FUJINET_BIN="$CONFIG_DIR/fujinet"
FUJINET_URL="0.0.0.0:8005"
FUJINET_BOIP_PORT=1987
```

| Variable          | Description                                                              |
|-------------------|--------------------------------------------------------------------------|
| CONFIG_DIR        | Location where your FujiNet fnconfig.ini file is, usually in build/dist  |
| VM_DIR            | Location of your 86Box directory with the 86box.cfg file                 |
| CONFIG_FILE       | Full path to your fnconfig.ini file                                      |
| FUJINET_BIN       | Name of fujinet executable, usually $CONFIG_DIR/fujinet                  |
| FUJINET_URL       | URL for Web UI. Port # should be unique                                  |
| FUJINET_BOIP_PORT | TCP port FujiNet's BoIP listener uses; must match 86box.cfg's `[FujiNet (BoIP) #1]` `port` |

If you run more than one VM at a time, give each a distinct
`FUJINET_BOIP_PORT` / `[FujiNet (BoIP) #1] port` pair (and a distinct
`FUJINET_URL` port) so their loopback listeners don't collide.

## Building 86Box

Build from the `feat/fujinet` branch of
[FujiNetWIFI/86Box](https://github.com/FujiNetWIFI/86Box) so the `fujinet`
character device is available:

```sh
git clone --branch feat/fujinet git@github.com:FujiNetWIFI/86Box.git
cmake -S 86Box -B 86Box/build -G Ninja -DQT=OFF
cmake --build 86Box/build
```

(`-DQT=OFF` is optional — the device works in either UI build. On distros
without a `gmake` binary, e.g. Arch, use `-G Ninja` — 86Box's default
Makefiles generator hardcodes `gmake`.)

## Building FujiNet-PC-RS232

FujiNet-PC must be built with the following parameters:

```
build.sh -p RS232 -b
```

## CONFIG.SYS inside host

The FUJINET.SYS file can be fetched from the `sys/` folder of this repository.

Be sure to use the correct COM port and baud rate, e.g. for most VMs, use the following:
```
DEVICE=FUJINET.SYS FUJI_PORT=1 FUJI_BPS=115200
```

or for PCjr:
```
DEVICE=FUJINET.SYS FUJI_PORT=2 FUJI_BPS=115200
```

## Desktop file to launch

The script can be copied to a name that matches your VM, e.g. __IBM_5160_PCXT.sh__
and referenced inside a .desktop file like this:

```sh
sudo cp launch-86box-and-fujinet.sh "/usr/local/bin/IBM 5150 PC.sh"
```

```desktop
[Desktop Entry]
Name=IBM 5150 PC
Exec="IBM 5150.sh"
Comment=Atari 8-bit Emulator
Terminal=false
StartupNotify=true
Icon=/home/thomc/Pictures/ibm-5150.png
StartupWMClass=86Box
Type=Application
Path=/home/thomc/Vintage/IBM 5150 PC
```
