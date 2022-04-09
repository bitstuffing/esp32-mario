
# ESP32-Mario Brick

This project is based on a FreeRTOS ESP32 Mario Brick with an ILI9341 screen (2,8").
It's developed with ESP-IDF 5.x (Jan 2022 master rev.) but it's compatible at
least versions after 3.x.

![ESP32-MARIO](https://i.ibb.co/Dz3gLJK/mario-brick.png)

The main idea of this project is offer a figure printed with a microcontroler
inside, like ESP32, and a simple easy available display to get an interactive cheap toy.

Why is not developed with Arduino or PlatformIO? Because there is an intention
to demonstrate the skills of the author and the knowledge acquired over the years,
but probably in the future, as time permits, it can be translated into other more
accessible frameworks for people who really want to learn how this type of device works.

## PinOut

They are defined in display.h, so you're able to change it.
This configuration works perfectly so use that, but, you're able to change it
at your risk:

| | ILI9341 | ESP32 |
|-- |--|--|
| 1 | VCC | 3.3v |
| 2 | GND | GND |
| 3 | CS | 5 |
| 4 | RESET | VCC (-1) |
| 5 | DC | 21 |
| 6 | SDI (MOSI) | 23 |
| 7 | SCK | 18 |
| 8 | LED | 14 |
| 9 | SDO (MISO) | 19 |

## Build process

It's based on a standard instalation so first step could be skipped:

    . $HOME/esp/esp-idf/export.sh

Building:

    idf.py build

Installing:

    idf.py -p /dev/ttyUSB0 flash

## DEBUG

    idf.py -p /dev/ttyUSB0 monitor


# LICENSE
All the code in all files, including display drivers, is released under CC 4.0 license,
developed by @bitstuffing with love. Basically you're able to learn, read, edit and
use that, but if you use that code you MUST reference the author.
Don't sell any fraction of this code, without the explicit permission of the author.
