# launch-86box-and-fujinet

## SYNOPSIS

This is a script that I use on my Arch Linux box to launch 86Box, grab the pts
that is allocated for the serial port, and write it into the fujinet-pc-rs232
directory.

## PREREQUISITES

You need the following

* FujiNet Firmware: https://github.com/FujiNetWIFI/fujinet-firmware
* 86Box: https://86box.net/
* 86Box-roms
* This repository's SYS/ directory for the CONFIG.SYS driver

## CONFIGURATION

Your desired 86Box VM's config file must have the following:

```cfg
[Serial Passthrough Device #1]
mode = 0
data_bits = 8
stop_bits = 1
baudrate = 9600

[Ports (COM & LPT)]
serial1_passthrough_enabled = 1
serial2_enabled = 0
```

__Note__: The PCjr emulation in 86Box is accurate enough to replicate the COM
port swapping that occurs, so you need to enable both COM1 and COM2, while
placing the pass-through on COM2, like this:

```cfg
[Serial Passthrough Device #1]
mode = 0
data_bits = 8
stop_bits = 1
baudrate = 9600

[Ports (COM & LPT)]
lpt1_enabled = 0
serial1_passthrough_enabled = 1
serial2_passthrough_enabled = 1

[Serial Passthrough Device #2]
mode = 0
data_bits = 8
stop_bits = 1
baudrate = 9600
```

Also, the top of the launch-86box-and-fujinet.sh script has a few important variables:

```sh
CONFIG_DIR="/home/thomc/Workspace/fujinet-pc-rs232/build/dist"
VM_DIR="/home/thomc/Vintage/IBM 4860 PCjr"
CONFIG_FILE="$CONFIG_DIR/fnconfig.ini"
VM_CONFIG_FILE="$VM_DIR/86box.cfg"
FUJINET_BIN="$CONFIG_DIR/fujinet"
FUJINET_URL="0.0.0.0:8005"
```

| Variable       | Description                                                             |
|----------------|-------------------------------------------------------------------------|
| CONFIG_DIR     | Location where your FujiNet fnconfig.ini file is, usually in build/dist |
| VM_DIR         | Location of your 86Box directory with the 86box.cfg file                |
| CONFIG_FILE    | Full path to your fnconfig.ini file                                     |
| VM_CONFIG_FILE | Full path of your 86box.cfg file                                        |
| FUJINET_BIN    | Name of fujinet executable, usually $CONFIG_DIR/fujinet                 |
| FUJINET_URL    | URL for Web UI. Port # should be unique                                 |

## Building FujiNet-PC-RS232

FujiNet-PC must be built with the following parameters:

```
build.sh -p RS232 -b
```

## CONFIG.SYS inside host

The FUJINET.SYS file can be fetched from the SYS/ folder of this repository.

Be sure to use the correct COM port and baud rate, e.g. for most VMs, use the following:
```
DEVICE=FUJINET.SYS FUJI_PORT=1 FUJI_BPS=9600
```

or for PCjr:
```
DEVICE=FUJINET.SYS FUJI_PORT=2 FUJI_BPS=9600
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
