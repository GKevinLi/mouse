# Wireless Mouse PCB README

<img width="603" height="842" alt="image" src="https://github.com/user-attachments/assets/3d951234-bac6-46f1-b1a1-54bdaf6f5a7d" />


This is the README for the project's PCB and KiCad files. Relevant information and known issues will be documented here.
An update will be coming soon to fix the current issues.

## Known Issues: 
- VBus Pin not connected to 5V, making USB flash not work on this version of the PCB. If ordering this board, this is the #1 thing that needs to be fixed in KiCad.
- GND and SCROLL_A are swapped on the encoder footprint
- NRESET pin, when connected to the nRF52833, causes erratic sensor issues on the PMW3610. The recommended solution for this is to either disconnect it from the microcontroller entirely, as it is not used, or physically cut the trace with a knife.

## Future Improvements:

I am planning on creating a second iteration of this PCB with some new features:
- Upgrade to nRF52840 chip
- Potential better mouse sensor
- Integrated battery charging IC
- LEDs
- Move ON/OFF switch to bottom for easier accessibility
