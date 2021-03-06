# pov-pi-led-driver
Persistence of vision display implementation - low-level app to drive LEDs in realtime using WiringPi and BCM2835 lib on Raspberry Pi Zero W. Provides frame buffer in shared memory and displays its contents. LED model: APA102.
# Notes
pov-pi-led-driver was created as a part of engineering diploma at PUT by Roman Oberenkowski. Device (hardware part) created by Roman Oberenkowski and Paweł Szalczyk. Photos and schematic by Paweł Szalczyk.

# Required libraries:
- WiringPi 2.50 <http://wiringpi.com/download-and-install/>
- BCM2835 1.71 <https://www.airspayce.com/mikem/bcm2835/>
# Configuration
Set parameters by editing header file led_constants.h. That includes:
- LED Count, vertical resolution - picture height
- COLUMNS_COUNT, horizontal resolution - picture "width"
# Compilation and launch:
Following commands will compile and run LED driver. Resulting binary requires higher privileges (because of BCM2835)
- Compile: `g++ −o leddrvexec leddriver.cpp −lbcm2835 −lwiringpi`
- Allow execution: `chmod u+x leddrvexec`
- Run with `sudo ./leddrvexec`

# Pics
<img src="https://github.com/roman-oberenkowski/pov-pi-led-driver/blob/ef2c6b48a703369f2ec867f1d18f1d5e3fc95101/readme_resources/pov_stationary.png" style="float: left; width: 75%; margin-right: 1%; margin-bottom: 0.5em;">
<img src="https://github.com/roman-oberenkowski/pov-pi-led-driver/blob/ef2c6b48a703369f2ec867f1d18f1d5e3fc95101/readme_resources/pov_test_pattern.jpg" style="float: left; width: 75%; margin-right: 1%; margin-bottom: 0.5em;">
<img src="https://github.com/roman-oberenkowski/pov-pi-led-driver/blob/ef2c6b48a703369f2ec867f1d18f1d5e3fc95101/readme_resources/schema.png" style="float: left; width: 75%; margin-right: 1%; margin-bottom: 0.5em;">
  
<p style="clear: both;">
