# FujiNet Tools - RS-232 for MS-DOS

MS-DOS utilities and drivers for the [FujiNet](https://github.com/FujiNetWIFI/fujinet-firmware) RS-232 adapter — an ESP32-based device that provides network access, virtual disk drives, and printer emulation to vintage computers via serial port.

## Components

| Output         | Description                                              |
|----------------|----------------------------------------------------------|
| `fujinet.sys`  | Block device driver — load in `CONFIG.SYS`               |
| `fujiprn.sys`  | Printer driver (redirects INT 17h to FujiNet)            |
| `fujicoms.lib` | FUJICOM RS-232 communications library                    |
| `fmount.exe`   | Mount and unmount FujiNet disk images as DOS drives      |
| `ncopy.exe`    | Interactive network file copy utility                    |
| `nget.exe`     | Download a file from a network URL                       |
| `fnshare.exe`  | TSR that mounts any share FujiNet can talk to as a drive |
| `setssid.exe`  | Configure FujiNet WiFi SSID and password                 |
| `iss.exe`      | ISS position tracker — HTTP/JSON demo                    |

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
make          # build all components
make clean    # remove all build artifacts
make builds   # build and copy outputs to builds/
make zip      # build and package outputs into fn-msdos.zip
make disk     # build and write a 1.44MB floppy image (fn-msdos.img)
make disk USE_GIT_REF=1  # same, but names the image fn-<git-hash>.img
```

`make disk` requires [mtools](https://www.gnu.org/software/mtools/) (`mformat`, `mcopy`).

## Further Reading

- [FUJICOM-Protocol.md](FUJICOM-Protocol.md) — RS-232 protocol specification (command frames, SLIP framing, pin assignments)
- [fujinet-bios.md](fujinet-bios.md) — INT F5 BIOS interface specification with C and assembly examples

## License

GPL v3 — see [license](license).
