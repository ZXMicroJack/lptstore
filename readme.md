LPTStore (c) 2023 Microjack
---------------------------

This is a project for the provision of a hard disk over the parallel port
for older XT machines with no hard disk.  The target for this project
was the AmstradPPC - which has the ISA bus mapped to connectors on the back
of the device, and there exist some excellent projects to connect an ISA
bus card containing an IDE hard disk controller, in turn connected to a 
CF flash disk.

This project simplifies this, using a Raspberry PI Pico (RP2040),
connected to the parallel port, and using a simple 6kb device driver
to communicate with it, which then has a USB flash disk connected
to it and serves 16 x 32MB hard disk images, labelled DISKA.DAT through
to DISKP.DAT.

The board is very simple and uses only 6 diodes, 6 10 k resistors, a 
hex inverter, a decoupling capacitor and of course the Pico.

Building
--------

To build the DOS device driver, first you need to download Borland C++ v3.0,
install using DOSbox, into a directory BORLANDC.  Then to build:

  cd ./dos
  dosbox -conf dosbox.conf

or to clean:

  cd ./dos
  dosbox -conf clean.conf
  
To build the Pico code, goto docker directory and build the container to
build.  It takes a specially modified version of TinyUSB, since the versions
that ship with the pico-sdk don't have HUD-MSC support.

  cd ./docker
  ./build.sh
  
Then use this to build the pico code.

  ./buildpico.sh
  
You can either burn this into the RP2040 by attaching a USB cable and holding
the program button, copying the lptstoreusb.uf2 file, but as soon as the device 
reboots, it will put the USB into host mode.  I'm fairly sure this will not kill 
your laptop, but I do highly recommend the SWD method using picoprobe.

  gdb-multiarch -ex "target remote localhost:3333" -ex "load" -ex "monitor reset init" 
      -ex "continue" pico/lptstoreusb.elf
  
Preparing the USB disk
----------------------

No partition parsing is done in the code - it expects one single
partition starting from LBA 0.  To create this in linux:

  sudo mkfs.vfat -F 32 /dev/sdb
  mkdir disk
  sudo mount /dev/sdb disk
  for n in A B C D E F G H I J K L M N O P; do
    fallocate -l 32M ./disk/DISK$n.DAT
    mkfs.vfat -F 16 ./disk/DISK$n.DAT
  done

Acknowledgements
----------------
As ever, we stand on the shoulders of giants, and I acknowledge my
sources.


DOS device driver 
-----------------
Copyright (C) 1994 by Robert Armstrong

This program is free software; you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation; either version 2 of the License, or 
(at your option) any later version.

This program is distributed in the hope that it will be useful, but  
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANT- 
ABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General 
Public License for more details.

You should have received a copy of the GNU General Public License 
along with this program; if not, visit the website of the Free 
Software Foundation, Inc., www.gnu.org.

TinyUSB
-------

The MIT License (MIT)

Copyright (c) 2018, hathach (tinyusb.org)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.


