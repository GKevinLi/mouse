# Wireless Mouse Firmware README


This is the README for the project's firmware. Relevant information and known issues will be documented here.

## How to Flash:

To flash the code onto the PCB, you either need a SWD debugger or a device running OpenOCD with SWD IO pins available.

I do not own a SWD debugger, so I'm not sure exactly how to flash with one of those. I have provided a .hex file to flash, so just follow your debugger's instructions on how to flash that and you should be good to go.

To set up OpenOCD on a Raspberry Pi (what I am using), I recommend this tutorial:
https://cdn-learn.adafruit.com/downloads/pdf/programming-microcontrollers-using-openocd-on-raspberry-pi.pdf
Once that is set up, paste the following into your openocd.cfg:

source [find interface/raspberrypi5-swd.cfg]\
transport select swd\
set CHIPNAME mouse\
source [find target/nrf52.cfg]\
reset_config srst_nogate\
adapter_nsrst_delay 100\
adapter_nsrst_assert_width 100\
init\
targets\
reset halt\
nrf5 mass_erase\
program HEX_NAME.hex verify\
reset\

Connect your mouse PCB's SWD pins to those of your Raspberry Pi's and run:\
sudo openocd\
and you're good to go! You should be able to see the mouse as a Bluetooth device and connect to it.



## Known Issues: 
- Bluetooth has trouble connecting sometimes
- Mouse sensor stops reporting movement occasionally
- Middle clicks do not work
- Drag clicking does not work


## Notes:
- This is a heavily modified version of the nRF Bluetooth Peripheral HIDS Mouse example. If there are issues flashing the mouse, try flashing the example as a sanity check.
- When modifying the code, these three lines must be in prj.conf or no code will work: CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC=y
CONFIG_CLOCK_CONTROL_NRF_K32SRC_XTAL=n
CONFIG_CLOCK_CONTROL_NRF_K32SRC_500PPM=y
- The firmware is very incomplete at the moment. Expect to see many future updates that will help mitigate some of the known issues.
