# TinyRV32
This is a small riscv emulator written for tiny MCUs and MCU boards like Arduino :)
## What is it?
This is a tiny riscv emulator for tiny MCUs (microcontrollers) for the RV32IM subset with nolibc (*yet).
## Why?
'cause why not ? :)
## What does it do?
It emulates riscv using very simplified C which makes it compatible with arduino ide.(just include the c file and write a frontend).
## PORTS
### ARDUINO UNO/ATMEGA328P based PORT
#### BASIC INFO
* RAM size of 1KB
* SW upload over UART
* Persistent RAM over resets(but ram is lost on poweroff)
* configurable system pins
* Python based uploader
* GPIO triggered upload
#### HOW TO'S
* Programming mode is triggered by pulling arduino uno pin D2 to GND
* CPU frequency of emulator can be found by shortly connecting and disconnecting the arduino uno pin D4 to GND.(Don't leave connected as it will lag the emulator)
#### QUICK START
* Plug in your UNO into your computer and flash the example Sketch from project repo.(CLONE THE ENTIRE REPO!!!)
* Take a jumper wire and connect one end into pin D2 and other to GND.
* Run the uploader python script.
* Disconnect jumper when script instructs you to.
* open a serial terminal of your choice and enjoy.
* Reset program by pressing arduino reset button.
* Program is stored in ram as long as there is power.(Using .noinit)
## Compiling apps
* Move to project sdk dir
* run make(You need to have the riscv toolchain installed!)
