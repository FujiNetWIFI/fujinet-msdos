# Description of the FujiNet RS232 Protocol

In order to get things running quickly for RS232, the Atari SIO protocol was adapted to run over RS232. This document describes the protocol contents and timing.

## RS232 Pin Assignments

| Pin | Description                                                 |
|-----|-------------------------------------------------------------|
| RXD | FujiNet Receive Data                                        |
| TXD | FujiNet Transmit Data                                       |
| DTR | Computer asserts this to indicate a command frame           |
| RI  | FujiNet asserts this to indicate available network traffic. |

## ESP32 GPIO Pin Assignments

The GPIO pins used on the ESP32 are listed below:

* see https://github.com/FujiNetWIFI/fujinet-firmware/blob/master/include/pinmap/rs232_rev0.h

``` C
#define PIN_UART1_RX            GPIO_NUM_13 // RS232
#define PIN_UART1_TX            GPIO_NUM_21 // RS232
#define PIN_RS232_DTR           GPIO_NUM_27 // (IN) Data Terminal Ready
#define PIN_RS232_RI            GPIO_NUM_32 // (OUT) Ring Indicator
```

## Default Serial Port Settings

Currently set in firmware: https://github.com/FujiNetWIFI/fujinet-firmware/blob/master/lib/bus/rs232/rs232.h#L9

* Speed: 9600bps
* Data Bits: 8
* Parity: None
* Stop Bits: 1

## Message Types

There are three types of messages that FujiNet can accept:

* No Payload. signals complete or error.
* Payload to Computer. Computer receives payload after command completes.
* Payload to FujiNet. FujiNet receives payload, executes command.

## Command Frame

In each of the three types, the protocol starts by the computer dropping DTRL low, and sending a command frame, consisting of the following 5 bytes:

| Offset | Size | Description               |
|--------|------|---------------------------|
| 0      | 1    | Device Number             |
| 1      | 1    | Command                   |
| 2      | 1    | aux1 (Parameter)          |
| 3      | 1    | aux2 (Parameter)          |
| 4      | 1    | 8-bit checksum, see below |

### Device Numbers

| Device | Description            |
|--------|------------------------|
| 0x31   | Disk Drive 1           |
| 0x32   | Disk Drive 2           |
| 0x33   | Disk Drive 3           |
| 0x34   | Disk Drive 4           |
| 0x35   | Disk Drive 5           |
| 0x36   | Disk Drive 6           |
| 0x37   | Disk Drive 7           |
| 0x38   | Disk Drive 8           |
| 0x40   | Printer                |
| 0x43   | Voice Synth            |
| 0x45   | Real Time Clock        |
| 0x50   | MODEM                  |
| 0x5A   | CP/M Emulation         |
| 0x70   | FujiNet Control Device |
| 0x71   | Network Device 1       |
| 0x72   | Network Device 2       |
| 0x73   | Network Device 3       |
| 0x74   | Network Device 4       |
| 0x75   | Network Device 5       |
| 0x76   | Network Device 6       |
| 0x77   | Network Device 7       |
| 0x78   | Network Device 8       |

### Commands

Commands for device 0x70: 
https://github.com/FujiNetWIFI/fujinet-firmware/wiki/SIO-Commands-for-Device-ID-%2470

Commands for device 0x71-0x78:
https://github.com/FujiNetWIFI/fujinet-firmware/wiki/SIO-Commands-for-Device-IDs-%2471-to-%2478

### Resend (command 0x05, any device)

A host that receives by polling a UART with no FIFO can lose part of a
reply while an interrupt it cannot mask runs (the PCjr's keyboard NMI holds
the CPU about 9 ms a key). Rather than run the command again (a network
read has already consumed its bytes), the host sends command 0x05 with no
aux bytes or payload to the same device; the FujiNet answers with its last
reply, byte for byte. FUJINET.SYS does this up to twice when a reply comes
short, garbled or for the wrong device, and gives up a reply 110 ms after
its last byte (its bytes come back to back). Supported by fujinet-pc-rs232
and firmware with the matching change; an older FujiNet ignores 0x05 and
the host sees its timeout as before.

### Calculating 8-bit Checksum

The 8 bit checksum is a simple sum, with any carry explicitly ignored. The implementation used by the firmware is shown here:

``` c++
// Calculate 8-bit checksum
uint8_t rs232_checksum(uint8_t *buf, unsigned short len)
{
    unsigned int chk = 0;

    for (int i = 0; i < len; i++)
        chk = ((chk + buf[i]) >> 8) + ((chk + buf[i]) & 0xff);

    return chk;
}
```

### Example: Reset FujiNet (by asking FUJI device)

``` txt
70 FF 00 00 6F
```

| Byte | Description                          |
|------|--------------------------------------|
| 0x70 | Device 0x70 - FujiNet Control device |
| 0xFF | Command - Reset                      |
| 0x00 | aux1 not used                        |
| 0x00 | aux2 not used                        |
| 0x6F | 8-bit checksum                       |

## Protocol Sequence

For each of the three types (no payload, payload->computer, payload->fujinet), the process is described below.

* Drop DTR
* Send command frame
* Raise DTR
* Wait for 'A'CK or 'N'AK byte, bail or retry if 'N'AK.
* If Payload->FujiNet, computer sends payload + checksum, waits for 'A'ck/'N'ak
* Computer waits for 'C'omplete or 'E'rror
* If Payload->Computer, FujiNet sends payload + checksum

