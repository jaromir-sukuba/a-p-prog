# Programmer for 8-bit PIC devices built upon AVR (or Arduino)


## Hardware
There are two options fo hardware for this project:

### Hardware option 1 - Arduino hardware
Take any Arduino with ATmega328P, like Uno or most of cheap chinese knock-off boards. Considering the target has its own power supply, connect GND, MCLR, PGC and PGD lines to respective pins on arduino as follows:

| Arduino pin | AVR pin | Target PIC pin | Comment                                                                     |
|-------------|---------|----------------|-----------------------------------------------------------------------------|
| GND         | GND     | GND            | common ground                                                               |
| 5V          | VDD     | VDD            | optional*- power supply for PIC MCU, you may power target from other source |
| A3          | PC3     | MCLR           | reset line of PIC MCU                                                       |
| A1          | PC1     | PGD            | programming data line                                                       |
| A0          | PC0     | PGC            | programming clock line                                                      |
|-------------|---------|----------------|-----------------------------------------------------------------------------|
* If you are powering target PIC from other power source (in circuit programming), series resistors (something like 470-1000 Ohms) in MCLR, PGC and PGD lines are good idea, it will somehow decrease currents flowing through IO pins due to power supplies differences between arduino and target board. In this configuration, even targets running at 3,3V are possible to program, though it is dirty practice.

Power it up, no smoke should be released. Run arduino IDE, open programmer firmware from here (/fw/pp.ino), complie and upload to arduino board - now you have PIC programmer ready to go. Go to software below.


### Hardware option 2 - dedicated board
I designed this little board, see at /hw directory
![img_4319](https://cloud.githubusercontent.com/assets/6984904/17289293/5cedd6c0-57d9-11e6-86b8-8d692eaa24e3.JPG)
Considering the target has its own power supply, connect GND, MCLR, PGC and PGD lines to respective pins on PIC programmer (notice the pinout similar to PICkit programmers). Vdd line is not currently used ad it is not needed for operation of programmer, but future revisions of firmware may take advantage of this pin and detect target VDD.
The hardware works with both FT232RL and CY7C65213 in place of USB/serial converter. Both were proven to work, with no other hardware difference.

Use [another arduino](https://www.arduino.cc/en/Tutorial/ArduinoISP) (or proper ISP programmer) to load Arduino UNO bootloader to PIC programmer board (performed onle once), turning it into regular arduino.

![img_4329](https://cloud.githubusercontent.com/assets/6984904/17289342/98207cf2-57d9-11e6-9b62-caba0b140ca5.JPG)

Ensure JP2 is closed, then you can load new firmware into PIC programmer using regular Arduino IDE. Open jumper JP2. Now you have your programmer ready to go, move on to software.



## Software

When running under Linux, download source from github and run

`gcc -Wall pp3.c -o pp3`

This should buiild executable pp2.

Under windows, you can download binary from here. Alternatively, you can build it from source - install minGW and run

`gcc -Wall pp3.c -o pp3`

- the same procedure as on Linux. This should result in silent build with pp3.exe executable created.

Running the executable with no parameters should only bring banner "pp programmer".

`./pp2 -c /dev/ttyACM0 -t 16f1829 file.hex`

under Linux, where -c parameter denotes port to be accessed, -t parameter defines PIC to be programmed and last parameter is binary file to be downloaded; or

`pp2.exe -c COM30 -t 16f1829 file.hex`

under Windows to run the actual software. And program the target PIC.

The result should look like this:

`$ ./pp2 -c /dev/ttyACM0 -t 16f1829 file.hex`

`Opening serial port`
`Device ID 0x27E4`
`Programming FLASH (16384 B in 256 pages)`
`Programming config`
`Verifying FLASH (16384 B in 256 pages)`
`Verifying config`
`Releasing MCLR`

And now your PIC should be programmed and running.

## Notes on software

If you are running the hardware on generic arduino board or you forget to open jumper JP2 after loading firmware on dedicated hardware, you may need to insert waiting time after opening serial port and before communication - to ensure Arduino bootloader times out and takes control to programmer firmware. It should look like this

`pp2.exe -c COM30 -s 1700 -t 16f1829 file.bin`

where number after -s switch defines the number of miliseconds to wait after opening the serial port.

You may omit the actual programming using -p switch or verification using -n switch, when using both the programmer only checks target device signature and exits.

`$ ./pp2 -c /dev/ttyACM0 -p -n -t 16f1829 file.bin`

`Opening serial port`
`Device ID 0x27E4`
`Releasing MCLR`

you can add some debug output info using -v parameter, ranging from -v 1 to -v 4


The whole project is licensed under MIT license, see LICENSE.md file.
some more details to be found here https://hackaday.io/project/8559-pic16f1xxx-arduino-based-programmer
