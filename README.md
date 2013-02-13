This emulator was created as part of a project to build a FPGA-based machine that could run CP/M. It would be based on a small FPGA with Z80, CPU, associated peripherals and VGA virtual terminal, and a microcontroller for the disk I/O. Hardware tself was never completed, but the emulator is quite complete.

# Prerequisites
You should have following installed in order to build and run the emulator:
* libz80ex - Z80 emulation library
* z80asm - Z80 assembler
* xterm

# Building
Emulator itself can be built in a normal autotools fashion by running:
* autoreconf -i
* ./configure
* make

I do believe that I have lost entire original CP/M source tree with the machine BIOS and such, so it would be difficult to build a disk image for the emulator. I do have one [on dropbox](https://www.dropbox.com/s/cjj6ao73cdih9lw/disk.img), but it lacks any fancy software.
