
# MegaDuino Firmware Readme

Original firmware writen by Andrew Beer, Duncan Edwards, Rafael Molina and others.

**MegaDuino Firmware** is an adaptation of the MaxDuino firmware by Rafael Molina and is specially designed for my MegaDuino PCB projects (Mega2560 & dual Mega2560/STM32).

**MegaDuino Firmware** have a different way of displaying information on OLED 128x64 1.3" (8 rows). Also you can use LCD 20x4 screens.

Thanks to the sixth button included in the MegaDuino project (board & components are cased) there is the possibility to do a reset of the board (whithout opening the case) or add extra functions (not implemented at the moment).

# ATTENTION

* All files must be copied to a folder named -> **MegaDuino_3.0**

# Versions

* Version 3.0 - February 8, 2025
   - Based on the latest OTLA version from rcmolina (https://github.com/rcmolina/MaxDuino/tree/master)
* Version 2.5 - May 1, 2023
   - Fixed some bugs on LCD & OLED displays
   - Some works done on File Types & Block IDs
   - Some works done on Menus
* Version 2.0 - September 20, 2022
  - Some bugs fixed when playing Amstrad CPC, Oric, ZX81 & MSX files
  - Better performance on I2C LCD & I2C OLED displays
  - Loading speed improved on Spectrum TZX turbo files, reaching speeds up to 5.100 bds with class 10 SD cards.
  - Possibility to select different 8x8 fonts to use with OLED screens
  - Some works done on options Menu
* Version 1.6 - October 1, 2021
  - Some works done with block rewind & forwarding (still in progress)
* Version 1.5 - September 11, 2021
  - Some optimizations for Arcon, Oric, Dragon & Camputerx Lynx
  - Corrected block counter for TSX files (MSX)
* Version 1.4 - August 16, 2021
  - Fixed some bugs on ID19 & ID21 Block Types
* Version 1.3 - March 3, 2021
  - Again .... Fixed some bugs on LCD screens
* Version 1.3 - February 27, 2021
  - Fixed some bugs with MSX CAS files
  - Timer optimized
* Version 1.2 - February 23, 2021
  - Fixed some bugs on LCD screens
* Version 1.2 - January 18, 2021
  - Performed some code debugging tasks
* Version 1.1 - January 14, 2021
  - Added logos & some bug fixes displaying ID Blocks
* Version 1.0 - January 8, 2021
  - Abandoned MaxDuino M versions & started job of MegaDuino Firmware


# Links

- https://www.winuaespanol.com/phpbb3/viewtopic.php?f=32&t=519
- https://www.va-de-retro.com/foros/viewtopic.php?f=63&t=5541
- https://github.com/rcmolina
