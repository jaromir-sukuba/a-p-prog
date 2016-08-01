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

* If you are powering target PIC from other power source (in circuit programming), series resistors (something like 470-1000 Ohms) in MCLR, PGC and PGD lines are good idea, it will somehow decrease currents flowing through IO pins due to power supplies differences between arduino and target board. In this configuration, even targets running at 3,3V are possible to program, though it is dirty practice.

Power it up, no smoke should be released. Run arduino IDE, open programmer firmware from here (/fw/pp.ino), complie and upload to arduino board - now you have PIC programmer ready to go. Go to software below.

The programmer is proven to work with some generic Uno board from china, as well as chinese arduino-nano clone
![img_5494](https://cloud.githubusercontent.com/assets/6984904/17290864/ddd9d386-57e0-11e6-8d15-55c6c0e015a6.JPG)

### Hardware option 2 - dedicated board
I designed this little board, see at /hw directory
![img_4319](https://cloud.githubusercontent.com/assets/6984904/17289293/5cedd6c0-57d9-11e6-86b8-8d692eaa24e3.JPG)
Considering the target has its own power supply, connect GND, MCLR, PGC and PGD lines to respective pins on PIC programmer (notice the pinout similar to PICkit programmers). Vdd line is not currently used ad it is not needed for operation of programmer, but future revisions of firmware may take advantage of this pin and detect target VDD.
The hardware works with both FT232RL and CY7C65213 in place of USB/serial converter. Both were proven to work, with no other hardware difference.
![img_4320](https://cloud.githubusercontent.com/assets/6984904/17289353/a3ccf6d4-57d9-11e6-9d4f-595633e7841a.JPG)

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

    Opening serial port
    Device ID 0x27E4
    Programming FLASH (16384 B in 256 pages)
    Programming config
    Verifying FLASH (16384 B in 256 pages)
    Verifying config
    Releasing MCLR

And now your PIC should be programmed and running.

## Notes on software

If you are running the hardware on generic arduino board or you forget to open jumper JP2 after loading firmware on dedicated hardware, you may need to insert waiting time after opening serial port and before communication - to ensure Arduino bootloader times out and takes control to programmer firmware. It should look like this

`pp2.exe -c COM30 -s 1700 -t 16f1829 file.bin`

where number after -s switch defines the number of miliseconds to wait after opening the serial port.

You may omit the actual programming using -p switch or verification using -n switch, when using both the programmer only checks target device signature and exits.

`$ ./pp2 -c /dev/ttyACM0 -p -n -t 16f1829 file.bin`

    Opening serial port
    Device ID 0x27E4
    Releasing MCLR

you can add some debug output info using -v parameter, ranging from -v 1 to -v 4


## Supported devices

Obviously, there is more supported than verified (tested) devices. I tried to test at least one device from specific family. The members from one family are usually very very similar, giving me enough confidence to mark them as supproted. Of course, typos can happen, but those are easy to correct.

|              |        | 
|--------------|--------| 
| DEVICE       | TESTED | 
| PIC12F1501   |        | 
| PIC12F1571   |        | 
| PIC12F1572   | YES    | 
| PIC12F1822   |        | 
| PIC12F1840   | YES    | 
| PIC12LF1501  |        | 
| PIC12LF1571  |        | 
| PIC12LF1572  |        | 
| PIC12LF1822  |        | 
| PIC12LF1840  |        | 
| PIC16F1454   |        | 
| PIC16F1455   | YES    | 
| PIC16F1459   |        | 
| PIC16F1503   | YES    | 
| PIC16F1507   | YES    | 
| PIC16F1508   |        | 
| PIC16F1509   | YES    | 
| PIC16F1824   |        | 
| PIC16F1825   |        | 
| PIC16F1826   |        | 
| PIC16F1827   |        | 
| PIC16F1828   |        | 
| PIC16F1829   | YES    | 
| PIC16F1847   |        | 
| PIC16LF1454  |        | 
| PIC16LF1455  |        | 
| PIC16LF1459  |        | 
| PIC16LF1503  |        | 
| PIC16LF1507  |        | 
| PIC16LF1508  |        | 
| PIC16LF1509  |        | 
| PIC16LF1824  |        | 
| PIC16LF1825  |        | 
| PIC16LF1826  |        | 
| PIC16LF1827  |        | 
| PIC16LF1828  |        | 
| PIC16LF1829  | YES    | 
| PIC16LF1847  |        | 
| PIC18F25K22  |        | 
| PIC18F25K50  | YES    | 
| PIC18F26K22  |        | 
| PIC18F45K22  |        | 
| PIC18F46K22  |        | 
| PIC18LF25K22 |        | 
| PIC18LF25K50 | YES    | 
| PIC18LF26K22 |        | 
| PIC18LF45K22 |        | 
| PIC18LF46K22 |        | 




The whole project is licensed under MIT license, see LICENSE.md file.
Some more details to be found here https://hackaday.io/project/8559-pic16f1xxx-arduino-based-programmer
