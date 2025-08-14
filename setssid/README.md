# setssid

A small utility that can set the SSID of your FujiNet from MS-DOS.

**NOTE**: FUJINET.SYS is required to be loaded in CONFIG.SYS

## Usage

```
setssid [ssid] [password] [/S] [/?]
```

where:

* [ssid] is an optional parameter specifying the SSID to use. (32 chars max)
* [password] is an optional parameter
* [/S] doesn't set an SSID but simply performs a scan and displays the results.
* [/?] Provides usage information and exits.

If no parameters are specified, then SETSSID will go into interactive mode.

If an SSID is specified, and no password is specified, then the user will be prompted for a password.

## Interactive Mode

If nothing is specified, then **setssid** will scan for networks. If no networks are found, then this will be indicated, and the program will exit.

If networks are found, then one will be displayed on each line, with an approximate signal strength:

```
   0: Cherryhomes          [***]
   1: Foo                  [ **]
   2: Bar                  [ **]
   3: FBI Van              [  *]

Enter SSID #, or
 'O' for other SSID, or
 [Enter] to abort. >> 
```

* Entering **O**, will allow you to specify a specific SSID, such as a hidden one.
* Pressing the **Enter** key will allow you to abort **setssid**
* and entering an SSID number, will select that SSID number.

It will then ask for a password:

```
Enter Password: ******
```

Once this is done, the FujiNet will attempt to set and connect to the SSID. If successful, you should see the FujiNet network light turn on, within 30 seconds.

If you do not see the FujiNet light turn on, re-run the program again.

## TODO

Add network status check to wait for network connection.

## Author

Thom Cherryhomes <thom.cherryhomes@gmail.com>

