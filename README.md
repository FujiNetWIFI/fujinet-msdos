# fujinet-rs232
Code for FujiNet RS-232 Version

## FUJICOM Environment Variables

The current code which is built against FUJICOM reacts to the following environment variables

| Variable  | Default | Description                         |
|-----------|---------|-------------------------------------|
| FUJI_PORT | 1       | The Serial port to use, 1, 2, 3, 4. |
| FUJI_BAUD | 9600    | The Baud rate to use.               |



## Build Directions

### Linux

wget https://github.com/open-watcom/open-watcom-v2/releases/download/Current-build/open-watcom-2_0-c-linux-x64

mkdir ~/openwatcom
cd ~/openwatcom
unzip ../Downloads/open-watcom-2_0-c-linux-x64


### Building

```
cd fujinet-rs232

$ make clean  - cleans all executables
$ make build  - makes all executatbles and put into builds directory
$ make zip   - makes all executables and puts them into a zip.
 
```
