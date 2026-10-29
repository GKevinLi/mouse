
# Mouse: A custom wireless Bluetooth mouse

<img width="540" height="720" alt="image" src="https://github.com/user-attachments/assets/cc9708f0-7d79-4880-818b-fae2d2e470ff" />

There is no cool name for it. I'm not good at naming things.

This is a wireless-capable Bluetooth mouse made entirely using custom parts. Everything required to make it is open source, and the files for each part are in their respective folders.
The only parts of this mouse that aren't custom are the battery and the various PCB attached components (switches, encoder, etc). 

This design can be used with any mouse switch, as long as it has dimensions of 12.8 x 5.8 x 6.5, and any 11mm mouse encoder. 6x6x7.3 silent switches theoretically should be able to work on this design as well, albeit with a few CAD modifications.

For more information on specific parts of the project, refer to the folder-specific READMEs in the cad, firmware, and pcb folders.

## Features

- PMW3610 Mouse Sensor
- nRF52833 Microcontroller
- Can be used when plugged in or on battery power
- Battery charging circuit
- Custom scroll wheel and mouse case
- 2 configurable side buttons
  

## Bill of Materials

| Item Name | Qty | Price (total) (USD) | Item link | Notes |
| --- | --- | --- | --- | --- |
| Mouse PCB, fully assembled | 2 | $150.05 | [Link](<https://jlcpcb.com/>) | this is how much it cost me for 5 PCBs, 2 fully assembled. Prices may vary. |
| PMW3610 | 1 | $1.09 | [Link](<https://www.aliexpress.us/item/3256809533648177.html?spm=a2g0o.productlist.main.7.117e61f1qj4GGu&algo_pvid=bf3782c9-0cfd-42bd-8a3f-8618d8d4e2c6&algo_exp_id=bf3782c9-0cfd-42bd-8a3f-8618d8d4e2c6-6&pdp_ext_f=%7B%22order%22%3A%22116%22%2C%22eval%22%3A%221%22%2C%22fromPage%22%3A%22search%22%7D&pdp_npi=6%40dis%21USD%214.53%211.09%21%21%214.53%211.09%21%402101e7a317932458066444948e0f08%2112000049947017292%21sea%21US%210%21ABX%211%210%21n_tag%3A-29910%3Bd%3A2859b2dd%3Bm03_new_user%3A-29895%3BpisId%3A5000000218152484&curPageLogUid=guoUMjTe3YSZ&utparam-url=scene%3Asearch%7Cquery_from%3A%7Cx_object_id%3A1005009719962929%7C_p_origin_prod%3A&_gl=1*7xkuy0*_gcl_au*MTIzNTAwODMyMy4xNzkzMjQ1ODEw*_ga*ODkxNjEwMTMuMTc5MzI0NTgxMg..*_ga_VED1YSGNC7*czE3OTMyNDU4MTEkbzEkZzAkdDE3OTMyNDU4MTEkajYwJGwwJGgw>) | this price is with the aliexpress welcome deal |
| Mouse Switch | 5 | $3.95 | [Link](<https://www.digikey.com/en/products/detail/aratas-formerly-omron-components-/D2FC-F-7N(60M)/20484328?gclsrc=aw.ds&gad_source=1&gad_campaignid=20509826117&gbraid=0AAAAADrbLlg5bJviKM8Id0HjudlkVKetQ&gclid=CjwKCAjwuobXBhBXEiwAYUaIlcOEJucEfbAOKAHeb95FXKGvGlaxJ1Z_NS2kmmfVS1ARsBkzc0fthhoCX_oQAvD_BwE>) | Any 12.8x5.8x6.5 mouse switch works, you can buy them or take them off old mice |
| 11mm Mouse Encoder | 1 | $9.39 | [Link](<https://www.amazon.com/dp/B07HCFMQ3X?plpRedirect=mhFallback&ref=clp_hp_h_pc>) | Any 11mm mouse encoder works, you can buy them or take them off old mice |
| M2 Screws | 6 |  |  |  |
| **Total Price (USD)** | | **$164.48** | | |

To build the mouse fully, you will need a soldering iron (and some soldering experience), an SWD debugger, and access to a 3D printer. The links given in the BOM are just recommendations. The battery is optional and the mouse can be used via USB power without it.


## Building tips / Notes:
- This project is currently in a very early stage of development, so building this is not recommended at this time. Only try to build this yourself if you REALLY know what you are doing. It is possible that your project may not work due to some oversight I haven't found yet.
- I am a very busy college student so updates on this may be slow.
- There are probably many issues with this design that I have not discovered yet. Documented issues will be in the folder-specific READMEs.
- The current board design has several major issues. These will have to either be fixed in KiCad or by modifying the assembled board. A corrected version of the design will be made.... eventually, but for now I have documented the issues in the PCB folder's readme file.
- The current mouse case does not have side buttons. These will be added later as well.
- The current mouse case does not support mouse skates.
- To build this, you will have to order your own boards from JLCPCB or another PCB manufacturer. Instructions for this will not be provided at the moment, since the board design has issues anyway. If you have enough experience to fix them then you probably have enough experience to order the boards by yourself as well.
- There is currently not a lot of documentation on this project. This is just a fun personal project of mine at the moment, and I do not have that much time to consistently work on it. I will update this in the future, though.
- THIS IS NOT (currently) A GOOD MOUSE. Do not expect this project, in its current form, to be better than whatever mouse you personally own.

## Credits:
https://github.com/badjeff/leylabella for CAD references

https://github.com/siderakb/pmw3610-pcb for a PMW3610 schematic reference

https://github.com/wareya/DIY-Gaming-Mouse for PCB design and CAD references



KiCanvas PCB Link Here: [![View PCB on KiCanvas](https://hack.club/pcb-badge)](https://kicanvas.org/?repo=https://github.com/GKevinLi/mouse/tree/main/pcb)
